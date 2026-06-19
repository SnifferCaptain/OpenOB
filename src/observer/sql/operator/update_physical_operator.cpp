#include "sql/operator/update_physical_operator.h"

#include <cstring>

#include "common/log/log.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  trx_ = trx;
  unique_ptr<PhysicalOperator> &child = children_[0];

  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("[update | physical]failed to open child operator: %s", strrc(rc));
    return rc;
  }

  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("[update | physical]failed to get current record: %s", strrc(rc));
      return RC::INTERNAL;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record copy;
    rc = copy.copy_data(row_tuple->record().data(), row_tuple->record().len());
    if (rc != RC::SUCCESS) {
      return rc;
    }
    copy.set_rid(row_tuple->record().rid());
    records_.emplace_back(std::move(copy));
  }
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("[update | physical]failed to fetch child tuple: %s", strrc(rc));
    return rc;
  }

  child->close();

  for (Record &record : records_) {
    rc = update_record(record);
    if (rc != RC::SUCCESS) {
      LOG_WARN("[update | physical]failed to update record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  records_.clear();
  trx_ = nullptr;
  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::update_record(Record &record)
{
  Record old_record;
  RC     rc = old_record.copy_data(record.data(), record.len());
  if (rc != RC::SUCCESS) {
    return rc;
  }
  old_record.set_rid(record.rid());

  Record new_record(old_record);
  for (const UpdateValueSqlNode &update_value : values_) {
    const FieldMeta *field = table_->table_meta().field(update_value.attribute_name.c_str());
    if (nullptr == field) {
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }

    rc = update_record_field(new_record, field, update_value.value);
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }

  if (memcmp(old_record.data(), new_record.data(), old_record.len()) == 0) {
    return RC::SUCCESS;
  }

  return trx_->update_record(table_, old_record, new_record);
}

RC UpdatePhysicalOperator::update_record_field(Record &record, const FieldMeta *field, const Value &value)
{
  Value real_value(value);
  if (field->type() != value.attr_type()) {
    RC rc = Value::cast_to(value, field->type(), real_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("[update | physical]failed to cast update value. field=%s, rc=%s", field->name(), strrc(rc));
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }

  if (field->type() == AttrType::CHARS) {
    record.reset_filed(field->offset(), field->len());
    int copy_len = real_value.length() + 1;
    if (copy_len > field->len()) {
      copy_len = field->len();
    }
    return record.set_field(field->offset(), copy_len, real_value.data());
  }

  return record.set_field(field->offset(), field->len(), real_value.data());
}
