#pragma once

class Table;
class FieldMeta;
class Value;
class Record;
#include <vector>

#include "sql/operator/physical_operator.h"

class Trx;
class UpdateStmt;

/**
 * @brief 物理算子，更新
 * @ingroup PhysicalOperator
 */
class UpdatePhysicalOperator : public PhysicalOperator {
public:
    UpdatePhysicalOperator(Table *table, const FieldMeta *field_meta, const Value *value)
        : table_(table), field_meta_(field_meta), value_(value) {}

    virtual ~UpdatePhysicalOperator() = default;

    /// @brief 获取算子类型
    PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }

    /// @brief 打开算子
    RC open(Trx *trx) override;

    /// @brief iter
    RC next() override;

    /// @brief 关闭算子
    RC close() override;

    /// @brief 获取当前元组
    Tuple *current_tuple() override { return nullptr; }
private:
    RC apply_update_to_record(Record &record, const Value &value);

    struct UpdateItem {
        Record old_record;
        Record new_record;
    };

    Table* table_ = nullptr;
    const FieldMeta* field_meta_ = nullptr;
    const Value* value_ = nullptr;
    Trx* trx_ = nullptr;
    std::vector<UpdateItem> update_items_;
};