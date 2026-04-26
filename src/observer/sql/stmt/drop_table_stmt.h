#pragma once

#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 删除表语句
 * 只保存表名，真正的删除动作交给执行阶段处理。
 * @ingroup Statement
 */
class DropTableStmt : public Stmt
{
public:
  DropTableStmt() = default;
  explicit DropTableStmt(const string &table_name) : table_name_(table_name) {}

  StmtType type() const override { return StmtType::DROP_TABLE; }

  /**
   * @brief 获取要删除的表名
   */
  const string &table_name() const { return table_name_; }

  /**
   * @brief 根据解析结果创建删除表语句
   * @param db 当前数据库
   * @param drop_table 解析得到的删除表节点
   * @param stmt 创建出来的语句对象
   */
  static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);

private:
  string table_name_;
};
