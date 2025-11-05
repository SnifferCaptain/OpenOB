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
#include "storage/field/field_meta.h"

#include "sql/operator/update_physical_operator.hpp"    // UPDATE物理算子 (physical)
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

    // 扫描算子,用于遍历表数据（简单实现：未考虑 WHERE 条件）
    auto scan_op = std::make_unique<TableScanPhysicalOperator>(table, ReadWriteMode::READ_WRITE);

    // 获取要更新的字段元信息和值
    const FieldMeta *field_meta = update_stmt->field_meta();
    if (field_meta == nullptr) {
        LOG_WARN("[UpdateExecutor::execute] field meta missing");
        return RC::SCHEMA_FIELD_MISSING;
    }

    // 构造 UPDATE 物理算子并挂载子算子
    const Value* value = update_stmt->values();
    if (value == nullptr) {
        LOG_WARN("[UpdateExecutor::execute] update value missing");
        return RC::INVALID_ARGUMENT;
    }
    auto update_op = std::make_unique<UpdatePhysicalOperator>(table, field_meta, value);
    update_op->add_child(std::move(scan_op));

    // 打开 UPDATE 算子，执行更新操作
    RC rc = update_op->open(trx);
    if (rc != RC::SUCCESS) {
        LOG_WARN("[UpdateExecutor::execute] failed to open update operator: %s", strrc(rc));
        return rc;
    }
    // 关闭算子释放资源
    update_op->close();
    return RC::SUCCESS;
}
// ********** TODO：fix this shit **********