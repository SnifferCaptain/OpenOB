#pragma once
#include <string>
#include <vector>
#include "sql/stmt/stmt.h"

class Db;

/**
 * @brief 删除表的语句
 * @ingroup Statement
 */
class DropTableStmt : public Stmt{
public:
    DropTableStmt(const std::string &table_name) : table_name_(table_name) {}
    virtual ~DropTableStmt() = default;
    StmtType type() const override { return StmtType::DROP_TABLE; }
    const std::string &table_name() const { return table_name_; }

    // @brief 创建 DropTableStmt 实例
    // @param db 数据库对象
    // @param drop_table 解析后的 DROP TABLE SQL 节点
    // @param stmt 返回的语句指针
    // @return RC 返回码，表示执行结果
    static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);
private:
    std::string table_name_;
};
