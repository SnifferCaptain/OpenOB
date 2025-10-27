#pragma once
#include "sql/expr/expression_tuple.h"
#include "sql/operator/physical_operator.h"
#include "sql/parser/parse.h"
#include "sql/stmt/update_stmt.h"
#include "storage/trx/trx.h"

// class UpdateStmt;

class UpdateOperator : public PhysicalOperator {
public:
    UpdateOperator(UpdateStmt *update_stmt, Trx *trx)
        : _update_stmt(update_stmt), _trx(trx) {}

    virtual ~UpdateOperator() = default;

    /// @brief 实现type()方法
    PhysicalOperatorType type() const override {
        return PhysicalOperatorType::UPDATE; // 没有update，暂时用INSERT代替
    }

    /// @brief 执行 UPDATE 操作符的主流程
    /// @param trx 当前事务指针
    /// @return RC 返回码，表示执行结果
    RC open(Trx *trx) override;

    /// @brief 获取下一个结果（更新操作符只做一次遍历，无需多次调用）
    /// @return RC 返回码，表示是否结束
    RC next() override;

    /// @brief 关闭 UpdateOperator，释放资源
    /// @return RC 返回码，表示执行结果
    RC close() override;

    /// @brief 获取当前 Tuple（行数据），实际由子算子提供
    /// @return Tuple* 当前行数据指针
    Tuple *current_tuple() override;

private:
    UpdateStmt *_update_stmt = nullptr;
    Trx *_trx = nullptr;
};