#include "sql/stmt/drop_index_stmt.h"

#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

using namespace common;

RC DropIndexStmt::create(Db *db, const DropIndexSqlNode &drop_index, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = drop_index.relation_name.c_str();
  const char *index_name = drop_index.index_name.c_str();
  if (db == nullptr || is_blank(table_name) || is_blank(index_name)) {
    LOG_WARN("invalid argument. db=%p, table_name=%s, index_name=%s", db, table_name, index_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  if (table->find_index(index_name) == nullptr) {
    LOG_WARN("no such index. table=%s, index=%s", table_name, index_name);
    return RC::NOTFOUND;
  }

  stmt = new DropIndexStmt(table, drop_index.index_name);
  return RC::SUCCESS;
}
