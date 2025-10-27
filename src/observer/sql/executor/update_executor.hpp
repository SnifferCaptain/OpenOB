#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @brief 更新表的执行器
 * @ingroup Executor
 */
class UpdateExecutor{
public:
    UpdateExecutor() = default;
    virtual ~UpdateExecutor() = default;
    
    // @brief 执行 UPDATE 语句的主流程
    // @param sql_event SQL 阶段事件，包含 SQL 语句和会话信息
    // @return RC 返回码，表示执行结果
    RC execute(SQLStageEvent *sql_event);
};
