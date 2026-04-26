#pragma once

#include "common/type/data_type.h"

/**
 * @brief 日期类型
 * @ingroup DataType
 * @details 内部使用 yyyymmdd 的整数编码，避免受系统时间范围影响。
 */
class DateType : public DataType
{
public:
  DateType() : DataType(AttrType::DATES) {}
  virtual ~DateType() = default;

  int compare(const Value &left, const Value &right) const override;
  int compare(const Column &left, const Column &right, int left_idx, int right_idx) const override;

  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  int cast_cost(AttrType type) override
  {
    if (type == AttrType::DATES) {
      return 0;
    } else if (type == AttrType::CHARS) {
      return 2;
    }
    return INT32_MAX;
  }

  RC set_value_from_str(Value &val, const string &data) const override;
  RC to_string(const Value &val, string &result) const override;

  static bool parse_date(const string &text, int &date_value);

private:
  static bool check_date(int year, int month, int day);
  static bool leap_year(int year);
};
