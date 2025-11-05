/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/table/lsm_table_engine.h"
#include "storage/record/heap_record_scanner.h"
#include "common/log/log.h"
#include "storage/index/bplus_tree_index.h"
#include "storage/common/meta_util.h"
#include "storage/db/db.h"
#include "storage/record/lsm_record_scanner.h"
#include "storage/common/codec.h"
#include "storage/trx/lsm_mvcc_trx.h"

RC LsmTableEngine::insert_record(Record &record)
{
  RC rc = RC::SUCCESS;
  // TODO: set auto increment id, and keep durability.
  // TODO: support set primary key as a part of lsm_key.
  bytes lsm_key;
  Codec::encode(table_->table_id(), inc_id_.fetch_add(1), lsm_key);
  rc = lsm_->put(string_view((char *)lsm_key.data(), lsm_key.size()), string_view(record.data(), record.len()));
  return rc;
}

RC LsmTableEngine::insert_record_with_trx(Record &record, Trx *trx)
{
  RC rc = RC::SUCCESS;
  
  // LSM 引擎使用自己的事务系统
  // LsmMvccTrx 会调用 ObLsmTransaction 的方法
  // 这里直接使用 lsm_->put，事务层会通过 LsmMvccTrx::insert_record 处理
  bytes lsm_key;
  Codec::encode(table_->table_id(), inc_id_.fetch_add(1), lsm_key);
  
  if (trx != nullptr && trx->type() == TrxKit::Type::LSM) {
    // LSM 事务通过 LsmMvccTrx 的 trx_ (ObLsmTransaction) 来处理
    // 实际上 LsmMvccTrx::insert_record 会调用这里，所以直接使用 lsm_->put
    rc = lsm_->put(string_view((char *)lsm_key.data(), lsm_key.size()), string_view(record.data(), record.len()));
  } else {
    rc = lsm_->put(string_view((char *)lsm_key.data(), lsm_key.size()), string_view(record.data(), record.len()));
  }
  
  if (rc == RC::SUCCESS) {
    // LSM 没有 RID 的概念，这里设置一个虚拟的 RID
    record.set_rid(RID(0, inc_id_.load() - 1));
  }
  
  return rc;
}

RC LsmTableEngine::delete_record_with_trx(const Record &record, Trx *trx)
{
  // LSM 引擎使用 key-value 模型，删除需要通过 key
  // 但是我们没有保存 record 到 key 的映射关系
  // 这是 LSM 引擎的一个限制：需要知道 key 才能删除
  
  // 目前的实现方案：
  // 1. LSM 引擎不支持基于 Record 的随机删除（因为没有 RID → key 的映射）
  // 2. 如果要支持，需要维护一个额外的索引或在 record 中存储 lsm_key
  
  LOG_WARN("LSM engine does not support delete_record_with_trx without key mapping");
  return RC::UNIMPLEMENTED;
}

RC LsmTableEngine::update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx)
{
  // LSM 引擎的 UPDATE = DELETE + INSERT
  // 但由于没有 RID → key 的映射，无法定位到具体的 key
  // 
  // 可能的解决方案：
  // 1. 在 record 中嵌入 lsm_key
  // 2. 维护一个 RID → lsm_key 的内存映射
  // 3. 使用主键作为 lsm_key（需要 schema 支持）
  
  LOG_WARN("LSM engine does not support update_record_with_trx without key mapping");
  return RC::UNIMPLEMENTED;
}

RC LsmTableEngine::get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode)
{
  scanner = new LsmRecordScanner(table_, db_->lsm(), trx);
  RC rc = scanner->open_scan();
  if (rc != RC::SUCCESS) {
    LOG_ERROR("failed to open scanner. rc=%s", strrc(rc));
  }
  return rc;
}

RC LsmTableEngine::open()
{
  return RC::UNIMPLEMENTED;
}