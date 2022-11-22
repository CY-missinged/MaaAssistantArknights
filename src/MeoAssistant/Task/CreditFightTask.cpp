#include "CreditFightTask.h"

#include <utility>

#include "TaskData.h"

#include "Plugin/DrGrandetTaskPlugin.h"
#include "Plugin/GameCrashRestartTaskPlugin.h"
#include "Plugin/StageDropsTaskPlugin.h"
#include "Sub/ProcessTask.h"
#include "CopilotTask.h"
#include "Sub/StageNavigationTask.h"
#include "Utils/AsstRanges.hpp"


asst::CreditFightTask::CreditFightTask(AsstCallback callback, void* callback_arg)
    : PackageTask(std::move(callback), callback_arg, TaskType),
      m_start_up_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_stage_navigation_task_ptr(std::make_shared<StageNavigationTask>(m_callback, m_callback_arg, TaskType)),
      m_fight_task_ptr(std::make_shared<ProcessTask>(m_callback, m_callback_arg, TaskType)),
      m_copilot_task_ptr(std::make_shared<CopilotTask>(m_callback, m_callback_arg))
{

    // 进入选关界面
    // 对于指定关卡，就是主界面的“终端”点进去
    // 对于当前/上次，就是点到 蓝色开始行动 为止。
    m_start_up_task_ptr->set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0)
        .set_times_limit("StageSNReturnFlag", 0)
        .set_times_limit("PRTS3", 0)
        .set_times_limit("PRTS", 0)
        .set_times_limit("PRTS2", 0)
        .set_times_limit("EndOfAction", 0)
        .set_ignore_error(false)
        .set_retry_times(5);

    m_stage_navigation_task_ptr->set_enable(false).set_ignore_error(false);

    // 战斗结束后
    m_fight_task_ptr->set_tasks({ "EndOfActionAndStop" }).set_ignore_error(false);

    m_stage_drops_plugin_ptr = m_fight_task_ptr->register_plugin<StageDropsTaskPlugin>();
    m_stage_drops_plugin_ptr->set_retry_times(0);
    m_game_restart_plugin_ptr = m_fight_task_ptr->register_plugin<GameCrashRestartTaskPlugin>();
    m_game_restart_plugin_ptr->set_retry_times(0);
    m_dr_grandet_task_plugin_ptr = m_fight_task_ptr->register_plugin<DrGrandetTaskPlugin>();
    m_dr_grandet_task_plugin_ptr->set_enable(false);

    m_subtasks.emplace_back(m_start_up_task_ptr);
    m_subtasks.emplace_back(m_stage_navigation_task_ptr);
    m_subtasks.emplace_back(m_copilot_task_ptr);
    m_subtasks.emplace_back(m_fight_task_ptr);
    
}

bool asst::CreditFightTask::set_params(const json::value& params)
{
    const std::string stage = "OF-1";
    const int medicine = 0;
    const int stone = 0;
    const int times = 0;
    bool enable_penguid = false;
    std::string penguin_id = params.get("penguin_id", "");
    std::string server = params.get("server", "CN");
    std::string client_type = params.get("client_type", std::string());
    bool is_dr_grandet = false;

    /*
    if (auto opt = params.find<json::object>("drops")) {
        std::unordered_map<std::string, int> drops;
        for (const auto& [item_id, quantity] : opt.value()) {
            drops.insert_or_assign(item_id, quantity.as_integer());
        }
        m_stage_drops_plugin_ptr->set_specify_quantity(drops);
    }
    */

    
    if (!m_running) {
        if (stage.empty()) {
            m_start_up_task_ptr->set_tasks({ "LastOrCurBattleBegin" }).set_times_limit("GoLastBattle", INT_MAX);
            m_stage_navigation_task_ptr->set_enable(false);
        }
        else {
            m_start_up_task_ptr->set_tasks({ "StageBegin" }).set_times_limit("GoLastBattle", 0);
            m_stage_navigation_task_ptr->set_stage_name(stage);
            m_stage_navigation_task_ptr->set_enable(true);
        }
        m_stage_drops_plugin_ptr->set_server(server);
    }
    

    
    m_fight_task_ptr->set_times_limit("MedicineConfirm", medicine)
        .set_times_limit("StoneConfirm", stone)
        .set_times_limit("StartButton1", times)
        .set_times_limit("StartButton2", times);
    

    m_dr_grandet_task_plugin_ptr->set_enable(is_dr_grandet);
    m_stage_drops_plugin_ptr->set_enable_penguid(enable_penguid);
    m_stage_drops_plugin_ptr->set_penguin_id(std::move(penguin_id));
    if (!client_type.empty()) {
        m_game_restart_plugin_ptr->set_client_type(client_type);
    }
    
    json::value copilotparams;
    copilotparams["stage_name"] = "activities/act3d0/level_act3d0_01";// OF-1
    copilotparams["filename"] = "resource/credit_fight_colilot.json";
    copilotparams["formation"] = true;
    m_copilot_task_ptr->set_params(copilotparams);

    return true;
}
