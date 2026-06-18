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
// Created by Wangyunlai.wyl on 2021/5/18.
//

#include "storage/index/index_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/field/field_meta.h"
#include "storage/table/table_meta.h"
#include "json/json.h"

const static Json::StaticString FIELD_NAME("name");
const static Json::StaticString FIELD_FIELD_NAME("field_name");
const static Json::StaticString FIELD_FIELD_NAMES("field_names");
const static Json::StaticString FIELD_UNIQUE("unique");

RC IndexMeta::init(const char *name, const FieldMeta &field, bool unique)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init index, name is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_  = name;
  field_ = field.name();
  fields_.clear();
  fields_.push_back(field.name());
  unique_ = unique;
  return RC::SUCCESS;
}

RC IndexMeta::init(const char *name, const vector<const FieldMeta *> &fields, bool unique)
{
  if (common::is_blank(name) || fields.empty()) {
    LOG_ERROR("Failed to init index, name or fields is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_ = name;
  fields_.clear();
  fields_.reserve(fields.size());
  for (const FieldMeta *field : fields) {
    if (field == nullptr) {
      return RC::INVALID_ARGUMENT;
    }
    fields_.push_back(field->name());
  }
  field_ = fields_.front();
  unique_ = unique;
  return RC::SUCCESS;
}

void IndexMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME]       = name_;
  json_value[FIELD_FIELD_NAME] = field_;
  Json::Value field_names;
  for (const string &field_name : fields_) {
    field_names.append(field_name);
  }
  json_value[FIELD_FIELD_NAMES] = std::move(field_names);
  json_value[FIELD_UNIQUE] = unique_;
}

RC IndexMeta::from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index)
{
  const Json::Value &name_value  = json_value[FIELD_NAME];
  const Json::Value &field_value = json_value[FIELD_FIELD_NAME];
  const Json::Value &fields_value = json_value[FIELD_FIELD_NAMES];
  const Json::Value &unique_value = json_value[FIELD_UNIQUE];
  if (!name_value.isString()) {
    LOG_ERROR("Index name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  bool unique = false;
  if (!unique_value.isNull()) {
    if (!unique_value.isBool()) {
      LOG_ERROR("Index unique flag is invalid. json value=%s", unique_value.toStyledString().c_str());
      return RC::INTERNAL;
    }
    unique = unique_value.asBool();
  }

  if (!fields_value.isNull()) {
    if (!fields_value.isArray() || fields_value.empty()) {
      LOG_ERROR("Field names of index [%s] is invalid. json value=%s",
          name_value.asCString(), fields_value.toStyledString().c_str());
      return RC::INTERNAL;
    }

    vector<const FieldMeta *> fields;
    fields.reserve(fields_value.size());
    for (const Json::Value &field_name_value : fields_value) {
      if (!field_name_value.isString()) {
        return RC::INTERNAL;
      }
      const FieldMeta *field = table.field(field_name_value.asCString());
      if (nullptr == field) {
        LOG_ERROR("Deserialize index [%s]: no such field: %s", name_value.asCString(), field_name_value.asCString());
        return RC::SCHEMA_FIELD_MISSING;
      }
      fields.push_back(field);
    }
    return index.init(name_value.asCString(), fields, unique);
  }

  if (!field_value.isString()) {
    LOG_ERROR("Field name of index [%s] is not a string. json value=%s",
        name_value.asCString(), field_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  const FieldMeta *field = table.field(field_value.asCString());
  if (nullptr == field) {
    LOG_ERROR("Deserialize index [%s]: no such field: %s", name_value.asCString(), field_value.asCString());
    return RC::SCHEMA_FIELD_MISSING;
  }

  return index.init(name_value.asCString(), *field, unique);
}

const char *IndexMeta::name() const { return name_.c_str(); }

const char *IndexMeta::field() const { return field_.c_str(); }

void IndexMeta::desc(ostream &os) const
{
  os << "index name=" << name_ << ", field=" << field_;
  if (unique_) {
    os << ", unique";
  }
}
