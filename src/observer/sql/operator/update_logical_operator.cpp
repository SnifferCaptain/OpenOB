#include "sql/operator/update_logical_operator.hpp"

UpdateLogicalOperator::UpdateLogicalOperator(UpdateStmt *update_stmt)
     : update_stmt_(update_stmt) {}