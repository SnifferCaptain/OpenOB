#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

class DropIndexExecutor
{
public:
  DropIndexExecutor()          = default;
  virtual ~DropIndexExecutor() = default;

  RC execute(SQLStageEvent *sql_event);
};
