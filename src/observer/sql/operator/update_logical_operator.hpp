#pragma once

#include "common/value.h"
#include "sql/operator/logical_operator.h"
#include "sql/stmt/update_stmt.h"

/**
 * @brief 逻辑算子，用于执行delete语句
 * @ingroup LogicalOperator
 */
class UpdateLogicalOperator : public LogicalOperator {
public:
    explicit UpdateLogicalOperator(UpdateStmt *update_stmt);
    virtual ~UpdateLogicalOperator() = default;

    LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }

    Table* table() const { return update_stmt_ != nullptr ? update_stmt_->table() : nullptr; }
    const FieldMeta *field_meta() const { return update_stmt_ != nullptr ? update_stmt_->field_meta() : nullptr; }
    const Value *value() const { return update_stmt_ != nullptr ? update_stmt_->values() : nullptr; }
    const char *attr_name() const { return update_stmt_ != nullptr ? update_stmt_->attr_name() : nullptr; }
    UpdateStmt* update_stmt() const { return update_stmt_; }

private:
    UpdateStmt *update_stmt_ = nullptr;
};
