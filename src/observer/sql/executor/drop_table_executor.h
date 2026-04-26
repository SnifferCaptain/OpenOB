#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @brief 删除表执行器
 * 负责把删除表语句转给当前数据库执行。
 * @ingroup Executor
 */
class DropTableExecutor
{
public:
  DropTableExecutor() = default;
  virtual ~DropTableExecutor() = default;

  /**
   * @brief 执行删除表命令
   * @param sql_event 当前 SQL 事件
   */
  RC execute(SQLStageEvent *sql_event);
};
