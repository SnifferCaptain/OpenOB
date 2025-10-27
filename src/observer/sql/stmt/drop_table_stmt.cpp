// 删除表语句实现
#include "common/log/log.h"         // 日志相关
#include "common/types.h"           // 通用类型定义
#include "event/sql_debug.h"        // SQL调试输出
#include "sql/stmt/drop_table_stmt.hpp" // 删除表语句头文件

RC DropTableStmt::create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt){
    // 创建 DropTableStmt 实例，保存表名
    stmt = new DropTableStmt(drop_table.relation_name);
    LOG_INFO("[DropTableStmt::create] table name %s", drop_table.relation_name.c_str());
    // 检查是否成功
    if (stmt == nullptr) {
        LOG_ERROR("[DropTableStmt::create] allocate DropTableStmt failed");
        return RC::NOMEM;
    } else {
        return RC::SUCCESS;
    }
}