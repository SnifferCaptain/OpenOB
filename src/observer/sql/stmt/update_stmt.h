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
#include "common/value.h"
#include "storage/field/field_meta.h"

class Table;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt{
public:
    UpdateStmt() = default;
    UpdateStmt(Table *table, const FieldMeta *field_meta, const Value &value, FilterStmt *filter_stmt);
    ~UpdateStmt() override;

    /// @brief 创建 UpdateStmt 对象
    /// @param db 数据库对象
    /// @param update SQL 解析节点
    /// @param stmt 输出参数，返回创建的语句对象
    /// @return 返回码，表示创建结果
    static RC create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt);

    /// @brief 获取表
    Table *table() const {return table_;}

    /// @brief 获取更新值
    const Value *values() const { return &value_; }
    const Value &value() const { return value_; }

    /// @brief 获取过滤语句
    FilterStmt *filter_stmt() const { return filter_stmt_; }

    /// @brief 获取语句类型
    StmtType type() const override { return StmtType::UPDATE; }

    /// @brief 获取字段名
    const char *attr_name() const { return field_meta_ != nullptr ? field_meta_->name() : nullptr; }

    /// @brief 字段数量（当前固定为1）
    int value_amount() const { return 1; }

    /// @brief 获取字段元信息
    const FieldMeta *field_meta() const { return field_meta_; }

private:
    Table* table_ = nullptr;
    Value value_;
    const FieldMeta *field_meta_ = nullptr;
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
