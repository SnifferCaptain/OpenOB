#include "common/log/log.h"                            // 日志相关
#include "sql/operator/physical_operator.h"            // 物理算子基类

#include "update_operator.hpp"                         // 更新操作符头文件

RC UpdateOperator::open(Trx *trx){
    LOG_DEBUG("[UpdateOperator::open] 打开喵");
    // 检查子算子数量，更新操作符必须有一个子算子
    if (children_.size() != 1u) {
        LOG_WARN("[UpdateOperator::open] update operator must has 1 child"); // 类型和数量不符，属于警告，保留
        return RC::INTERNAL;
    }

    // 打开子算子，准备遍历数据
    PhysicalOperator *child = children_[0].get();
    RC rc = child->open(trx);
    if (rc != RC::SUCCESS) {
        LOG_WARN("[UpdateOperator::open] failed to open child operator: %s", strrc(rc)); // 打开失败，属于警告，保留
        return rc;
    }
    Table *table = _update_stmt->table();

    LOG_DEBUG("[UpdateOperator::open] 更新洗耙子！");
    // 遍历所有满足条件的记录，逐条更新
    while (RC::SUCCESS == (rc = child->next())) {
        Tuple *tuple = child->current_tuple();
        if (nullptr == tuple) {
            LOG_WARN("[UpdateOperator::open] failed to get current record: %s", strrc(rc)); // 获取失败，属于警告，保留
            return rc;
        }
        // 获取行记录并执行更新
        RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
        Record &record = row_tuple->record();
        const Value *value = _update_stmt->value();
        rc = table->update_record(_trx, &record, _update_stmt->attr_name(), value);
        if (rc != RC::SUCCESS) {
            LOG_WARN("[UpdateOperator::open] failed to update record: %s", strrc(rc)); // 更新失败，属于警告，保留
            return rc;
        }
    }
    return rc;
}

RC UpdateOperator::next() {
    return RC::RECORD_EOF;
}

RC UpdateOperator::close(){
    LOG_DEBUG("[UpdateOperator::close] 关闭喵");
    if (!children_.empty()) {
        children_[0]->close();
    }
    return RC::SUCCESS;
}

Tuple *UpdateOperator::current_tuple() {
    if (children_.empty()) {
        // 如果没有子操作符，直接返回空指针
        return nullptr;
    }
    PhysicalOperator *child = children_[0].get();
    return child->current_tuple();
}