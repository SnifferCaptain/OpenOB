/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Wangyunlai on 2021/5/13.
//

#include <limits.h>
#include <string.h>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/lang/span.h"
#include "common/lang/algorithm.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "storage/db/db.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/common/condition_filter.h"
#include "storage/common/meta_util.h"
#include "storage/index/bplus_tree_index.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "storage/record/heap_record_scanner.h"
#include "storage/record/lsm_record_scanner.h"
#include "storage/table/heap_table_engine.h"
#include "storage/table/lsm_table_engine.h"

Table::~Table()
{
  if (lob_handler_ != nullptr) {
    delete lob_handler_;
    lob_handler_ = nullptr;
  }
}

RC Table::create(Db *db, int32_t table_id, const char *path, const char *name, const char *base_dir,
    span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format, StorageEngine storage_engine)
{
  if (table_id < 0) {
    LOG_WARN("invalid table id. table_id=%d, table_name=%s", table_id, name);
    return RC::INVALID_ARGUMENT;
  }

  if (common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to create table %s:%s", base_dir, name);

  if (attributes.size() == 0) {
    LOG_WARN("Invalid arguments. table_name=%s, attribute_count=%d", name, attributes.size());
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  // 使用 table_name.table记录一个表的元数据
  // 判断表文件是否已经存在
  int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (EEXIST == errno) {
      LOG_ERROR("Failed to create table file, it has been created. %s, EEXIST, %s", path, strerror(errno));
      return RC::SCHEMA_TABLE_EXIST;
    }
    LOG_ERROR("Create table file failed. filename=%s, errmsg=%d:%s", path, errno, strerror(errno));
    return RC::IOERR_OPEN;
  }

  close(fd);

  // 创建文件
  const vector<FieldMeta> *trx_fields = db->trx_kit().trx_fields();
  if ((rc = table_meta_.init(table_id, name, trx_fields, attributes, primary_keys, storage_format, storage_engine)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init table meta. name:%s, ret:%d", name, rc);
    return rc;  // delete table file
  }

  fstream fs;
  fs.open(path, ios_base::out | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", path, strerror(errno));
    return RC::IOERR_OPEN;
  }

  // 记录元数据到文件中
  table_meta_.serialize(fs);
  fs.close();

  db_       = db;

  string             data_file = table_data_file(base_dir, name);
  BufferPoolManager &bpm       = db->buffer_pool_manager();
  rc                           = bpm.create_file(data_file.c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }

  if (table_meta_.storage_engine() == StorageEngine::HEAP) {
    engine_ = make_unique<HeapTableEngine>(&table_meta_, db_, this);
  } else if (table_meta_.storage_engine() == StorageEngine::LSM) {
    engine_ = make_unique<LsmTableEngine>(&table_meta_, db_, this);
  } else {
    rc = RC::UNSUPPORTED;
    LOG_WARN("Unsupported storage engine type: %d", table_meta_.storage_engine());
    return rc;
  }
  rc = engine_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open table %s due to engine open failed.", data_file.c_str());
    return rc;
  }

  LOG_INFO("Successfully create table %s:%s", base_dir, name);
  return rc;
}

RC Table::open(Db *db, const char *meta_file, const char *base_dir)
{
  // 加载元数据文件
  fstream fs;
  string  meta_file_path = string(base_dir) + common::FILE_PATH_SPLIT_STR + meta_file;
  fs.open(meta_file_path, ios_base::in | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open meta file for read. file name=%s, errmsg=%s", meta_file_path.c_str(), strerror(errno));
    return RC::IOERR_OPEN;
  }
  if (table_meta_.deserialize(fs) < 0) {
    LOG_ERROR("Failed to deserialize table meta. file name=%s", meta_file_path.c_str());
    fs.close();
    return RC::INTERNAL;
  }
  fs.close();

  db_       = db;

  // // 加载数据文件
  // RC rc = init_record_handler(base_dir);
  // if (rc != RC::SUCCESS) {
  //   LOG_ERROR("Failed to open table %s due to init record handler failed.", base_dir);
  //   // don't need to remove the data_file
  //   return rc;
  // }
  RC rc = RC::SUCCESS;

  if (table_meta_.storage_engine() == StorageEngine::HEAP) {
    engine_ = make_unique<HeapTableEngine>(&table_meta_, db_, this);
  }  else if (table_meta_.storage_engine() == StorageEngine::LSM) {
    engine_ = make_unique<LsmTableEngine>(&table_meta_, db_, this);
  } else {
    rc = RC::UNSUPPORTED;
    LOG_ERROR("Unsupported storage engine type: %d", table_meta_.storage_engine());
    return rc;
  }

  rc = engine_->open();
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open table %s due to engine open failed.", base_dir);
    return rc;
  }

  return rc;
}

RC Table::insert_record(Record &record)
{
  return engine_->insert_record(record);
}

RC Table::insert_chunk(const Chunk& chunk)
{
  return engine_->insert_chunk(chunk);
}

RC Table::visit_record(const RID &rid, function<bool(Record &)> visitor)
{
  return engine_->visit_record(rid, visitor);
}

RC Table::insert_record_with_trx(Record &record, Trx *trx)
{
  return engine_->insert_record_with_trx(record, trx);
}
RC Table::delete_record_with_trx(const Record &record, Trx *trx)
{
  return engine_->delete_record_with_trx(record, trx);
}

RC Table::update_record_with_trx(const Record &old_record, const Record &new_record, Trx* trx)
{
  return engine_->update_record_with_trx(old_record, new_record, trx);
}

RC Table::get_record(const RID &rid, Record &record)
{
  return engine_->get_record(rid, record);
}

const char *Table::name() const { return table_meta_.name(); }

const TableMeta &Table::table_meta() const { return table_meta_; }

RC Table::make_record(int value_num, const Value *values, Record &record)
{
  RC rc = RC::SUCCESS;
  // 检查字段类型是否一致
  if (value_num + table_meta_.sys_field_num() != table_meta_.field_num()) {
    LOG_WARN("Input values don't match the table's schema, table name:%s", table_meta_.name());
    return RC::SCHEMA_FIELD_MISSING;
  }

  const int normal_field_start_index = table_meta_.sys_field_num();
  // 复制所有字段的值
  int   record_size = table_meta_.record_size();
  char *record_data = (char *)malloc(record_size);
  memset(record_data, 0, record_size);

  for (int i = 0; i < value_num && OB_SUCC(rc); i++) {
    const FieldMeta *field = table_meta_.field(i + normal_field_start_index);
    const Value &    value = values[i];
    if (field->type() != value.attr_type()) {
      Value real_value;
      rc = Value::cast_to(value, field->type(), real_value);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to cast value. table name:%s,field name:%s,value:%s ",
            table_meta_.name(), field->name(), value.to_string().c_str());
        break;
      }
      rc = set_value_to_record(record_data, real_value, field);
    } else {
      rc = set_value_to_record(record_data, value, field);
    }
  }
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to make record. table name:%s", table_meta_.name());
    free(record_data);
    return rc;
  }

  record.set_data_owner(record_data, record_size);
  return RC::SUCCESS;
}

RC Table::set_value_to_record(char *record_data, const Value &value, const FieldMeta *field)
{
  size_t       copy_len = field->len();
  const size_t data_len = value.length();
  if (field->type() == AttrType::CHARS) {
    if (copy_len > data_len) {
      copy_len = data_len + 1;
    }
  }
  memcpy(record_data + field->offset(), value.data(), copy_len);
  return RC::SUCCESS;
}

RC Table::get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode)
{
  return engine_->get_record_scanner(scanner, trx, mode);
}

RC Table::get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode)
{
  return engine_->get_chunk_scanner(scanner, trx, mode);
}

RC Table::create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name)
{
  return engine_->create_index(trx, field_meta, index_name);
}

RC Table::delete_record(const Record &record)
{
  return engine_->delete_record(record);
}

Index *Table::find_index(const char *index_name) const
{
  return engine_->find_index(index_name);
}
Index *Table::find_index_by_field(const char *field_name) const
{
  return engine_->find_index_by_field(field_name);
}

RC Table::sync()
{
  return engine_->sync();
}
/////// SC's modification migrated to engine-based implementation ///////

RC Table::remove(const char *name) {
  if (common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to remove table %s:%s", db_->path().c_str(), name);

  // 先同步并销毁引擎，释放索引和 buffer 等资源（HeapTableEngine 的析构会释放 indexes_ 等）
  if (engine_) {
    (void)engine_->sync();
    engine_.reset();
  }

  // 删除索引文件
  for (int i = 0; i < table_meta_.index_num(); i++) {
    const IndexMeta *index_meta = table_meta_.index(i);
    std::string index_file = table_index_file(db_->path().c_str(), name, index_meta->name());
    if (0 != ::unlink(index_file.c_str())) {
      LOG_ERROR("Delete index file failed. filename=%s, errmsg=%d:%s", index_file.c_str(), errno, strerror(errno));
      // continue trying to remove other files, but return error
      return RC::IOERR_OPEN;
    }
  }

  // 删除数据文件
  std::string data_file = table_data_file(db_->path().c_str(), name);
  if (0 != ::unlink(data_file.c_str())) {
    LOG_ERROR("Delete data file failed. filename=%s, errmsg=%d:%s", data_file.c_str(), errno, strerror(errno));
    return RC::IOERR_OPEN;
  }

  // 删除元数据文件
  std::string meta_file = table_meta_file(db_->path().c_str(), name);
  if (0 != ::unlink(meta_file.c_str())) {
    LOG_ERROR("Delete meta file failed. filename=%s, errmsg=%d:%s", meta_file.c_str(), errno, strerror(errno));
    return RC::IOERR_OPEN;
  }

  LOG_INFO("Successfully removed table %s:%s", db_->path().c_str(), name);
  return RC::SUCCESS;
}

// 更新记录（按字段更新）：通过 engine_->visit_record 在页面锁保护下修改记录内容，随后维护索引
RC Table::update_record(Trx *trx, Record *record, const char *attribute_name, const Value *values) {
  LOG_INFO("Begin to update record. table name=%s, attribute name=%s", table_meta_.name(), attribute_name);

  if (record == nullptr || attribute_name == nullptr || values == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  const FieldMeta *field = table_meta_.field(attribute_name);
  if (field == nullptr) {
    LOG_ERROR("Field not found. table=%s, field=%s", table_meta_.name(), attribute_name);
    return RC::SCHEMA_FIELD_MISSING;
  }

  const Value &value = values[0];
  // 类型检查
  if (field->type() != value.attr_type()) {
    LOG_ERROR("Invalid value type. table name=%s, field name=%s, expect=%d, given=%d",
              table_meta_.name(), field->name(), field->type(), value.attr_type());
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  // 计算拷贝长度
  size_t copy_len = field->len();
  if (field->type() == AttrType::CHARS) {
    const size_t data_len = strlen((const char *)value.data());
    if (copy_len > data_len) {
      copy_len = data_len + 1;
    }
  }

  // 备份旧数据（用于索引维护）
  std::string old_data;
  if (record->data() != nullptr) {
    old_data.assign(record->data(), table_meta_.record_size());
  }

  // 构造新数据副本（不修改传入的 record，使用副本来更新索引）
  std::string new_data = old_data;
  if (new_data.size() < static_cast<size_t>(table_meta_.record_size())) {
    new_data.resize(table_meta_.record_size());
  }
  memcpy(&new_data[0] + field->offset(), value.data(), copy_len);

  // 在页面锁下修改记录内容（engine_->visit_record 会在合适的页上拿到写锁并调用 page_handler->update_record）
  RC rc = engine_->visit_record(record->rid(), [&](Record &r) -> bool {
    // r 是拷贝出来的 record，可以直接修改并返回 true 表示需要写回
    if (r.data() == nullptr) {
      LOG_ERROR("Invalid inplace record data. table=%s, rid=%s", table_meta_.name(), r.rid().to_string().c_str());
      return false;
    }
    // 修改拷贝中的字段
    memcpy(r.data() + field->offset(), value.data(), copy_len);
    return true; // 告诉底层需要写回页面
  });

  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to update record in page. rc=%s", strrc(rc));
    return rc;
  }

  // 更新索引：先插入新索引项，再删除旧索引项（其实可以考虑类似swap的优化？）
  for (int i = 0; i < table_meta_.index_num(); i++) {
    const IndexMeta *index_meta = table_meta_.index(i);
    if (0 == strcmp(index_meta->field(), attribute_name)) {
      Index *idx = engine_->find_index(index_meta->name());
      if (idx == nullptr) {
        LOG_WARN("index not found while updating record. table=%s, index=%s", table_meta_.name(), index_meta->name());
        continue;
      }
      // 插入新键
      RC rc2 = idx->insert_entry(new_data.c_str(), &record->rid());
      if (rc2 != RC::SUCCESS) {
        LOG_WARN("failed to insert_entry while updating index. table=%s, index=%s, rc=%s",
                 table_meta_.name(), index_meta->name(), strrc(rc2));
        // 如果插入失败，这里不做复杂回滚，只记录日志
        continue;
      }
      // 删除旧键
      rc2 = idx->delete_entry(old_data.c_str(), &record->rid());
      if (rc2 != RC::SUCCESS) {
        LOG_WARN("failed to delete_entry while updating index. table=%s, index=%s, rc=%s",
                 table_meta_.name(), index_meta->name(), strrc(rc2));
      }
      LOG_INFO("Succeed to update index (field=%s) of record(rid=%d.%d).",
               index_meta->field(), record->rid().page_num, record->rid().slot_num);
    }
  }

  return RC::SUCCESS;
}