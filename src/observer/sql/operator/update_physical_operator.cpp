#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "common/log/log.h"
#include "common/value.h"
#include "sql/operator/physical_operator.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

#include "sql/operator/update_physical_operator.hpp"

RC UpdatePhysicalOperator::open(Trx *trx)
{
    if (children_.empty()) {
        return RC::SUCCESS;
    }

    if (nullptr == table_ || nullptr == field_meta_ || nullptr == value_) {
        LOG_WARN("invalid operator state: table=%p field_meta=%p value=%p", table_, field_meta_, value_);
        return RC::INVALID_ARGUMENT;
    }

    update_items_.clear();

    unique_ptr<PhysicalOperator> &child = children_[0];

    RC rc = child->open(trx);
    if (OB_FAIL(rc)) {
        LOG_WARN("failed to open child operator: %s", strrc(rc));
        return rc;
    }

    trx_ = trx;

    const int record_size = table_->table_meta().record_size();

    Value cast_value;
    const Value *update_value = value_;
    if (value_->attr_type() != field_meta_->type()) {
        rc = Value::cast_to(*value_, field_meta_->type(), cast_value);
        if (OB_FAIL(rc)) {
            LOG_WARN("failed to cast update value. field=%s value=%s rc=%s", field_meta_->name(), value_->to_string().c_str(), strrc(rc));
            child->close();
            return rc;
        }
        update_value = &cast_value;
    }

    while (OB_SUCC(rc = child->next())) {
        Tuple *tuple = child->current_tuple();
        if (nullptr == tuple) {
            LOG_WARN("failed to get current tuple");
            child->close();
            return RC::INTERNAL;
        }

        RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
        Record &source_record = row_tuple->record();

        UpdateItem item;
        rc = item.old_record.copy_data(source_record.data(), record_size);
        if (OB_FAIL(rc)) {
            LOG_WARN("failed to clone source record: %s", strrc(rc));
            child->close();
            return rc;
        }
        item.old_record.set_rid(source_record.rid());
        item.old_record.set_key(source_record.key());

        item.new_record = item.old_record;

        rc = apply_update_to_record(item.new_record, *update_value);
        if (OB_FAIL(rc)) {
            LOG_WARN("failed to apply update value: %s", strrc(rc));
            child->close();
            return rc;
        }

        update_items_.emplace_back(std::move(item));
    }

    if (rc != RC::RECORD_EOF) {
        LOG_WARN("failed to iterate child operator: %s", strrc(rc));
        child->close();
        return rc;
    }

    child->close();

    for (UpdateItem &item : update_items_) {
        rc = trx_->update_record(table_, item.old_record, item.new_record);
        if (OB_FAIL(rc)) {
            LOG_WARN("failed to update record: %s", strrc(rc));
            return rc;
        }
    }

    return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next() { return RC::RECORD_EOF; }

RC UpdatePhysicalOperator::close()
{
    update_items_.clear();
    trx_ = nullptr;
    return RC::SUCCESS;
}

RC UpdatePhysicalOperator::apply_update_to_record(Record &record, const Value &value)
{
    if (nullptr == field_meta_) {
        LOG_WARN("field meta is null");
        return RC::INVALID_ARGUMENT;
    }

    const int field_len = field_meta_->len();
    if (field_len <= 0) {
        LOG_WARN("invalid field length. field=%s len=%d", field_meta_->name(), field_len);
        return RC::INVALID_ARGUMENT;
    }

    const int field_offset = field_meta_->offset();
    std::vector<char> buffer(field_len, 0);

    int copy_len = std::min(field_len, value.length());
    if (field_meta_->type() == AttrType::CHARS && field_len > 0 && copy_len >= field_len) {
        copy_len = field_len - 1;
    }

    if (copy_len > 0) {
        memcpy(buffer.data(), value.data(), copy_len);
    }

    RC rc = record.set_field(field_offset, field_len, buffer.data());
    if (OB_FAIL(rc)) {
        LOG_WARN("failed to set field data. field=%s rc=%s", field_meta_->name(), strrc(rc));
    }
    return rc;
}
