#pragma once

#include "sql/stmt/stmt.h"

class Table;

/**
 * @brief 删除索引语句
 * @ingroup Statement
 */
class DropIndexStmt : public Stmt
{
public:
  DropIndexStmt(Table *table, const string &index_name) : table_(table), index_name_(index_name) {}

  StmtType type() const override { return StmtType::DROP_INDEX; }

  Table *table() const { return table_; }
  const string &index_name() const { return index_name_; }

  static RC create(Db *db, const DropIndexSqlNode &drop_index, Stmt *&stmt);

private:
  Table *table_ = nullptr;
  string index_name_;
};
