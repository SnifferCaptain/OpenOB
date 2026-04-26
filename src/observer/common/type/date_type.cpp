#include "common/type/date_type.h"

#include "common/lang/comparator.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/value.h"
#include "storage/common/column.h"

int DateType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::DATES && right.attr_type() == AttrType::DATES, "invalid date type");
  return common::compare_int((void *)&left.value_.int_value_, (void *)&right.value_.int_value_);
}

int DateType::compare(const Column &left, const Column &right, int left_idx, int right_idx) const
{
  ASSERT(left.attr_type() == AttrType::DATES && right.attr_type() == AttrType::DATES, "invalid date type");
  return common::compare_int((void *)&((int *)left.data())[left_idx], (void *)&((int *)right.data())[right_idx]);
}

RC DateType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    case AttrType::DATES: {
      result.set_date(val.value_.int_value_);
      return RC::SUCCESS;
    }
    case AttrType::CHARS: {
      string date_text;
      RC rc = to_string(val, date_text);
      if (rc != RC::SUCCESS) {
        return rc;
      }
      result.set_string(date_text.c_str());
      return RC::SUCCESS;
    }
    default: {
      LOG_WARN("unsupported type %d", type);
      return RC::SCHEMA_FIELD_TYPE_MISMATCH;
    }
  }
}

RC DateType::set_value_from_str(Value &val, const string &data) const
{
  int date_value = 0;
  if (!parse_date(data, date_value)) {
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  val.set_date(date_value);
  return RC::SUCCESS;
}

RC DateType::to_string(const Value &val, string &result) const
{
  int date_value = val.value_.int_value_;
  int year = date_value / 10000;
  int month = date_value / 100 % 100;
  int day = date_value % 100;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);
  result = buffer;
  return RC::SUCCESS;
}

bool DateType::parse_date(const string &text, int &date_value)
{
  int year = 0;
  int month = 0;
  int day = 0;
  char tail = '\0';

  if (sscanf(text.c_str(), "%d-%d-%d%c", &year, &month, &day, &tail) != 3) {
    return false;
  }

  if (!check_date(year, month, day)) {
    return false;
  }

  date_value = year * 10000 + month * 100 + day;
  return true;
}

bool DateType::check_date(int year, int month, int day)
{
  static const int days_per_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year <= 0 || month < 1 || month > 12 || day < 1) {
    return false;
  }

  int max_day = days_per_month[month];
  if (month == 2 && leap_year(year)) {
    max_day = 29;
  }
  return day <= max_day;
}

bool DateType::leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}
