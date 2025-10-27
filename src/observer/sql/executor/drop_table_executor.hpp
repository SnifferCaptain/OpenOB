#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @brief 删除表的执行器
 * @ingroup Executor
 */
class DropTableExecutor{
public:
    DropTableExecutor() = default;
    virtual ~DropTableExecutor() = default;

    // @brief 执行 DROP TABLE 语句
    // @param sql_event SQL 阶段事件，包含 SQL 语句和会话信息
    // @return RC 返回码，表示执行结果
    RC execute(SQLStageEvent *sql_event);
};