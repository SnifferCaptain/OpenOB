#include "sql/stmt/drop_table_stmt.h"

#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

RC DropTableStmt::create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt)
{
  const char *table_name = drop_table.relation_name.c_str();
  if (db == nullptr || table_name == nullptr) {
    LOG_WARN("invalid argument. db=%p, table_name=%p", db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  // 先在语句层确认表存在，避免执行阶段才返回更晚的错误。
  Table *table = db->find_table(table_name);
  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // 语句对象只保存表名，后续执行阶段直接使用。
  stmt = new DropTableStmt(drop_table.relation_name);
  return RC::SUCCESS;
}
