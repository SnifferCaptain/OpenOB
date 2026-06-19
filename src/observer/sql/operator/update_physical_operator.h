/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/value.h"
#include "sql/operator/physical_operator.h"
#include "sql/parser/parse_defs.h"
#include "storage/record/record.h"

class Table;
class FieldMeta;

/**
 * @brief 物理算子，更新
 * @ingroup PhysicalOperator
 */
class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, const vector<UpdateValueSqlNode> &values) : table_(table), values_(values)
  {}

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }

  /**
   * @brief 执行更新
   * @param trx 当前事务
   */
  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;
  Tuple *current_tuple() override { return nullptr; }

private:
  RC update_record(Record &record);
  RC update_record_field(Record &record, const FieldMeta *field, const Value &value);

private:
  Table *table_ = nullptr;
  vector<UpdateValueSqlNode> values_;
  vector<Record> records_;
  Trx *trx_ = nullptr;
};
