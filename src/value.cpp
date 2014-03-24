/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <boost/lexical_cast.hpp>

#include "value.h"

Value::Value()
    : value(NullTag())
{
}

Value::Value(NullTag)
    :  value(NullTag())
{
}

Value::Value(UndefinedTag)
    :  value(UndefinedTag())
{
}


Value::Value(bool b)
    : value(b)
{
}

Value::Value(uint32_t i)
    : value(static_cast<int64_t>(i))
{
}

Value::Value(int32_t i)
    : value(static_cast<int64_t>(i))
{
}

Value::Value(int64_t i)
    : value(i)
{
}

Value::Value(uint64_t i)
    : value(static_cast<int64_t>(i))
{
}

Value::Value(double d)
    : value(d)
{
}

Value::Value(const std::string &s)
    : value(s)
{
}

Value::Value(const char *s)
    : value(std::string(s))
{
}

Value::Value(const std::vector<Value> &vec)
    : value(vec)
{
}

Value::Value(const std::map<Value, Value> &map)
    : value(map)
{
}

bool Value::isNull() const
{
    return type() == Null;
}

bool Value::isUndefined() const
{
    return type() == Undefined;
}

bool Value::isBool() const
{
    return type() == Bool;
}

bool Value::isInt() const
{
    return type() == Integer;
}

bool Value::isDouble() const
{
    return type() == Double;
}

bool Value::isString() const
{
    return type() == String;
}

bool Value::isArray() const
{
    return type() == Array;
}

bool Value::isMap() const
{
    return type() == Map;
}

bool Value::toBool() const
{
    return castTo<bool>();
}

int Value::toInt() const
{
    return castTo<int64_t>();
}

int64_t Value::toInt64() const
{
    return castTo<int64_t>();
}

double Value::toDouble() const
{
    return castTo<double>();
}

std::string Value::toString() const
{
    return castTo<std::string>();
}

std::vector<Value> Value::toArray() const
{
    return castTo< std::vector<Value> >();
}

std::map<Value, Value> Value::toMap() const
{
    return castTo< std::map<Value, Value> >();
}

Value::Type Value::type() const
{
    return static_cast<Value::Type>(value.which());
}

std::string Value::inspect() const
{
    if( isNull() )
    {
        static std::string null = "(null)";
        return null;
    }
    else if( isUndefined() )
    {
        static std::string undefined = "(undefined)";
        return undefined;
    }
    else if( isBool() )
    {
        return boost::lexical_cast<std::string>(toBool());
    }
    else if( isInt() )
    {
        return boost::lexical_cast<std::string>(toInt());
    }
    else if( isDouble() )
    {
        return boost::lexical_cast<std::string>(toDouble());
    }
    else if( isString() )
    {
        return toString();
    }
    else if( isArray() )
    {
        std::vector<Value> values = toArray();
        std::string result = "[";

        if( values.empty() == false )
        {
            for(size_t i = 0; i < values.size(); ++i)
            {
                result += values[i].inspect();
                result += ", ";
            }

            result.resize(result.size() - 1);
            result[result.size() - 1] = ']';
        }
        else
        {
            result += ']';
        }

        return result;
    }
    else if( isMap() )
    {
        std::map<Value, Value> values = toMap();
        std::string result = "{";

        if( values.empty() == false )
        {
            std::map<Value, Value>::iterator it = values.begin();
            std::map<Value, Value>::iterator end = values.end();

            for(; it != end; ++it)
            {
                result += it->first.inspect();
                result += ": ";
                result += it->second.inspect();
                result += ", ";
            }

            result.resize(result.size() - 1);
            result[result.size() - 1] = '}';
        }
        else
        {
            result += '}';
        }

        return result;
    }
    else
    {
        assert(false);
        std::string invalidType = "(invalid type)";
        return invalidType;
    }
}

bool operator < (const Value &lhs, const Value &rhs)
{
    return lhs.value < rhs.value;
}

bool operator == (const Value &lhs, const Value &rhs)
{
    return lhs.value == rhs.value;
}

