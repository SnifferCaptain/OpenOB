/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#pragma once

#include "common/sys/rc.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/filter_stmt.h"

class Table;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt{
public:
    UpdateStmt() = default;
    UpdateStmt(Table *table, const char *attr_name, const Value *value, FilterStmt *filter_stmt);
    ~UpdateStmt() override;

    // @brief 创建 UpdateStmt 对象
    // @param db 数据库对象
    // @param update SQL 解析节点
    // @param stmt 输出参数，返回创建的语句对象
    // @return 返回码，表示创建结果
    static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

    // 获取表名
    Table *table() const {return table_;}

    // 获取属性值
    const Value *value() const { return value_; }

    // 获取过滤语句
    FilterStmt *filter_stmt() const { return filter_stmt_; }

    // 获取语句类型
    StmtType type() const override { return StmtType::UPDATE; }

    // 获取属性名
    const char *attr_name() const { return attr_name_; }
private:
    Table *table_ = nullptr;
    const Value *value_ = nullptr;
    const char *attr_name_ = nullptr;
    FilterStmt *filter_stmt_ = nullptr;
};

// old
// class UpdateStmt : public Stmt
// {
// public:
//   UpdateStmt() = default;
//   UpdateStmt(Table *table, Value *values, int value_amount);

// public:
//   static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

// public:
//   Table *table() const { return table_; }
//   Value *values() const { return values_; }
//   int    value_amount() const { return value_amount_; }

// private:
//   Table *table_        = nullptr;
//   Value *values_       = nullptr;
//   int    value_amount_ = 0;
// };
