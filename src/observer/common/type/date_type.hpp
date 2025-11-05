#pragma once

#include "common/type/data_type.h"

/**
 * @brief 日期类型
 * @ingroup DataType
 */
class DateType : public DataType{
public:
    DateType() : DataType(AttrType::DATES) {}
    virtual ~DateType() {}

    /// @brief 比较两个值
    int compare(const Value &left, const Value &right) const override;

    /// @brief 加法
    RC add(const Value &left, const Value &right, Value &result) const override;

    /// @brief 减法
    RC subtract(const Value &left, const Value &right, Value &result) const override;

    /// @brief 乘法
    RC multiply(const Value &left, const Value &right, Value &result) const override;

    /// @brief 取负数
    RC negative(const Value &val, Value &result) const override;

    /// @brief 从字符串设置值
    RC set_value_from_str(Value &val, const string &data) const override;

    /// @brief 转换成字符串
    RC to_string(const Value &val, string &result) const override;
};
