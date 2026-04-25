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

#include "sql/stmt/update_stmt.h"

#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

UpdateStmt::UpdateStmt(Table *table, const string &attribute_name, const Value &value, FilterStmt *filter_stmt)
    : table_(table), attribute_name_(attribute_name), value_(value), filter_stmt_(filter_stmt)
{
}

UpdateStmt::~UpdateStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update_sql, Stmt *&stmt)
{
  const char *table_name = update_sql.relation_name.c_str();
  const char *field_name = update_sql.attribute_name.c_str();
  if (nullptr == db || update_sql.relation_name.empty() || update_sql.attribute_name.empty()) {
    LOG_WARN("[update | stmt] 输入无效. db=%p, table_name=%p, attribute=%s", db, table_name, field_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("[update | stmt] 表不存在. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *field = table->table_meta().field(field_name);
  if (nullptr == field) {
    LOG_WARN("[update | stmt] 字段不存在. table=%s, field=%s", table_name, field_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  Value value(update_sql.value);
  if (field->type() != value.attr_type()) {
    Value cast_value;
    RC rc = Value::cast_to(update_sql.value, field->type(), cast_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("[update | stmt] 类型不匹配 table=%s, field=%s, value=%s, rc=%s",
          table_name, field_name, update_sql.value.to_string().c_str(), strrc(rc));
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
    value = cast_value;
  }

  FilterStmt *filter_stmt = nullptr;
  unordered_map<string, Table *> table_map;
  table_map.emplace(update_sql.relation_name, table);

  RC rc = FilterStmt::create(db,
      table,
      &table_map,
      update_sql.conditions.data(),
      static_cast<int>(update_sql.conditions.size()),
      filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("[update | stmt] 创建过滤语句失败. rc=%d:%s", rc, strrc(rc));
    return rc;
  }

  stmt = new UpdateStmt(table, field_name, value, filter_stmt);
  return RC::SUCCESS;
}
