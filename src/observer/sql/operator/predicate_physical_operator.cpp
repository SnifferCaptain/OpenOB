/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2022/6/27.
//

#include "sql/operator/predicate_physical_operator.h"
#include "common/log/log.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/field/field.h"
#include "storage/record/record.h"

static void force_predicate_integer_division(std::unique_ptr<Expression> &expr)
{
  if (expr == nullptr) {
    return;
  }

  switch (expr->type()) {
    case ExprType::ARITHMETIC: {
      auto *arithmetic_expr = static_cast<ArithmeticExpr *>(expr.get());
      if (arithmetic_expr->arithmetic_type() == ArithmeticExpr::Type::DIV &&
          arithmetic_expr->left()->type() == ExprType::FIELD &&
          arithmetic_expr->right() != nullptr &&
          arithmetic_expr->right()->value_type() == AttrType::INTS) {
        arithmetic_expr->force_integer_division();
      }
      force_predicate_integer_division(arithmetic_expr->left());
      force_predicate_integer_division(arithmetic_expr->right());
    } break;
    case ExprType::COMPARISON: {
      auto *comparison_expr = static_cast<ComparisonExpr *>(expr.get());
      force_predicate_integer_division(comparison_expr->left());
      force_predicate_integer_division(comparison_expr->right());
    } break;
    case ExprType::CONJUNCTION: {
      auto *conjunction_expr = static_cast<ConjunctionExpr *>(expr.get());
      for (std::unique_ptr<Expression> &child : conjunction_expr->children()) {
        force_predicate_integer_division(child);
      }
    } break;
    case ExprType::CAST: {
      auto *cast_expr = static_cast<CastExpr *>(expr.get());
      force_predicate_integer_division(cast_expr->child());
    } break;
    case ExprType::FUNCTION: {
      auto *function_expr = static_cast<FunctionExpr *>(expr.get());
      for (std::unique_ptr<Expression> &child : function_expr->children()) {
        force_predicate_integer_division(child);
      }
    } break;
    default: break;
  }
}

PredicatePhysicalOperator::PredicatePhysicalOperator(std::unique_ptr<Expression> expr) : expression_(std::move(expr))
{
  force_predicate_integer_division(expression_);
  ASSERT(expression_->value_type() == AttrType::BOOLEANS, "predicate's expression should be BOOLEAN type");
}

RC PredicatePhysicalOperator::open(Trx *trx)
{
  if (children_.size() != 1) {
    LOG_WARN("predicate operator must has one child");
    return RC::INTERNAL;
  }

  return children_[0]->open(trx);
}

RC PredicatePhysicalOperator::next()
{
  RC                rc   = RC::SUCCESS;
  PhysicalOperator *oper = children_.front().get();

  while (RC::SUCCESS == (rc = oper->next())) {
    Tuple *tuple = oper->current_tuple();
    if (nullptr == tuple) {
      rc = RC::INTERNAL;
      LOG_WARN("failed to get tuple from operator");
      break;
    }

    Value value;
    rc = expression_->get_value(*tuple, value);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    if (value.get_boolean()) {
      return rc;
    }
  }
  return rc;
}

RC PredicatePhysicalOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

Tuple *PredicatePhysicalOperator::current_tuple() { return children_[0]->current_tuple(); }

RC PredicatePhysicalOperator::tuple_schema(TupleSchema &schema) const
{
  return children_[0]->tuple_schema(schema);
}
