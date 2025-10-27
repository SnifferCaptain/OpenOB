#include "common/log/log.h"              // 日志相关
#include "common/type/attr_type.h"       // 属性类型定义
#include "event/session_event.h"         // 会话事件
#include "event/sql_event.h"             // SQL事件
#include "session/session.h"             // 会话管理
#include "storage/db/db.h"               // 数据库相关

#include "sql/stmt/drop_table_stmt.hpp"
#include "sql/executor/drop_table_executor.hpp"

RC DropTableExecutor::execute(SQLStageEvent *sql_event){
    // 获取 SQL 语句对象
    Stmt *stmt = sql_event->stmt();
    // 获取当前会话
    Session *session = sql_event->session_event()->session();
    ASSERT(stmt->type() == StmtType::DROP_TABLE,
        "drop table executor can not run this command: %d",
        static_cast<int>(stmt->type())
    );
    // 转换为 DropTableStmt 类型，获取表名
    // DropTableStmt *drop_table_stmt = static_cast<DropTableStmt *>(stmt);
    // const char *table_name = drop_table_stmt->table_name().c_str();
    // RC rc = session->get_current_db()->drop_table(table_name);
    RC rc = session->get_current_db()->drop_table(static_cast<DropTableStmt *>(stmt)->table_name().c_str());
    return rc;
}