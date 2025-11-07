#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/value.h"
#include <sstream>
#include <iomanip>
#include <cstdio>

#include "common/type/date_type.hpp"
#include "common/utils/date_utils.hpp"

int DateType::compare(const Value &left, const Value &right) const {
    ASSERT(left.attr_type() == AttrType::DATES && right.attr_type() == AttrType::DATES, "Invalid type");
    int intl = left.get_int();
    int intr = right.get_int();
    return intl - intr;
}

RC DateType::add(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

// 可以考虑支持下？
RC DateType::subtract(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

RC DateType::multiply(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

RC DateType::negative(const Value &val, Value &result) const { return RC::UNSUPPORTED; }

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    case AttrType::DATES: {
      result = val;
      return RC::SUCCESS;
    }
    case AttrType::CHARS: {
      // 将日期转换为字符串
      string date_str;
      RC rc = to_string(val, date_str);
      if (rc != RC::SUCCESS) {
        return rc;
      }
      result.set_string(date_str.c_str());
      return RC::SUCCESS;
    }
    default: return RC::UNIMPLEMENTED;
  }
}

int DateType::cast_cost(AttrType type)
{
  if (type == AttrType::DATES) {
    return 0;
  }
  if (type == AttrType::CHARS) {
    return 1;  // DATE 可以转换为字符串，成本为 1
  }
  return INT32_MAX;
}

RC DateType::set_value_from_str(Value &val, const string &data) const{
    // 解析日期字符串 "YYYY-MM-DD" 或 "YYYY-M-D"
    int year, month, day;
    int parsed = sscanf(data.c_str(), "%d-%d-%d", &year, &month, &day);
    
    if (parsed != 3) {
        LOG_WARN("Failed to parse date string: %s", data.c_str());
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
    
    // 验证日期有效性
    if (!check_date(year, month, day)) {
        LOG_WARN("Invalid date: %04d-%02d-%02d", year, month, day);
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
    
    // 转换为整数格式 YYYYMMDD
    int date_int = year * 10000 + month * 100 + day;
    val.set_date(date_int);
    
    return RC::SUCCESS;
}

RC DateType::to_string(const Value &val, string &result) const{
  int date_int = val.get_int();
  std::stringstream ss;
  ss << std::setw(4) << std::setfill('0') << date_int / 10000 << "-" 
     << std::setw(2) << std::setfill('0') << (date_int / 100 % 100) << "-" 
     << std::setw(2) << std::setfill('0') << (date_int % 100);
  result = ss.str();
  return RC::SUCCESS;
}
