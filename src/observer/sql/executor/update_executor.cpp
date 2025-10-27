#include <memory>
#include "common/type/data_type.h"                      // 数据类型定义
#include "common/log/log.h"                             // 日志相关
#include "common/types.h"                               // 通用类型定义
#include "common/type/attr_type.h"                      // 属性类型定义
#include "event/session_event.h"                        // 会话事件
#include "event/sql_event.h"                            // SQL事件
#include "session/session.h"                            // 会话管理
#include "sql/stmt/update_stmt.h"                       // UPDATE语句结构体
#include "storage/db/db.h"                              // 数据库相关
#include "storage/record/record_log.h"                  // 记录日志
#include "sql/expr/expression.h"                        // 表达式相关
#include "sql/operator/table_scan_physical_operator.h"  // 表扫描物理算子
#include "sql/operator/predicate_physical_operator.h"   // 谓词物理算子

#include "sql/operator/update_operator.hpp"               // UPDATE物理算子
#include "sql/executor/update_executor.hpp"             // UPDATE执行器

RC UpdateExecutor::execute(SQLStageEvent *sql_event) {
    // 获取 SQL 语句对象
    Stmt *stmt = sql_event->stmt();
    // SessionEvent *session_event = sql_event->session_event();
    // Session *session = session_event->session();
    // Trx *trx = session->current_trx(); // 当前事务
    auto* trx = sql_event->session_event()->session()->current_trx();

    // 检查语句是否为空
    if (stmt == nullptr) {
        LOG_WARN("[UpdateExecutor::execute] cannot find statement");
        return RC::INVALID_ARGUMENT;
    }

    // 转换为 UpdateStmt 类型，获取表对象
    UpdateStmt *update_stmt = static_cast<UpdateStmt *>(stmt);
    Table *table = update_stmt->table();

    // 扫描算子,用于遍历表数据
    auto scan_op = std::make_unique<TableScanPhysicalOperator>(table, ReadWriteMode::READ_WRITE);

    // 构造一个 ValueExpr（此处仅为示例，实际应根据 UPDATE 语句设置值）
    Value value;
    value.set_type(AttrType::INTS); // 设置类型为整数
    auto value_expr = std::make_unique<ValueExpr>(value);

    // 谓词物理算子,用于过滤数据，实际应根据WHERE条件构造
    auto pred_op = std::make_unique<PredicatePhysicalOperator>(std::move(value_expr));
    pred_op->add_child(std::move(scan_op));

    // 构造UPDATE算子继续嵌套
    auto update_op = std::make_unique<UpdateOperator>(update_stmt, trx);
    update_op->add_child(std::move(pred_op));

    // 打开 UPDATE 算子，执行更新操作
    RC rc = update_op->open(trx);
    if (rc != RC::SUCCESS) {
        LOG_WARN("[UpdateExecutor::execute] failed to open update operator: %s", strrc(rc));
        return rc;
    }
    return rc;
}