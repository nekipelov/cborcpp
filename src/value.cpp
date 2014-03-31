/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <limits>
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

Value::Value(int i)
{
    if( i >= 0 )
    {
        PositiveInteger num = {static_cast<uint64_t>(i)};
        value = num;
    }
    else
    {
        NegativeInteger num = {static_cast<uint64_t>(-i)};
        value = num;
    }
}

Value::Value(int64_t i)
{
    if( i >= 0 )
    {
        PositiveInteger num = {static_cast<uint64_t>(i)};
        value = num;
    }
    else
    {
        NegativeInteger num = {static_cast<uint64_t>(-i)};
        value = num;
    }
}

Value::Value(uint64_t i, bool positive)
{
    if( positive )
    {
        PositiveInteger num = {i};
        value = num;
    }
    else
    {
        NegativeInteger num = {i};
        value = num;
    }
}

Value::Value(double d)
    : value(d)
{
}

Value::Value(const std::string &s)
    : value(s)
{
}

Value::Value(const std::vector<char> &bs)
    : value(bs)
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

Value::Value(const BigInteger &bigint)
    : value(bigint)
{
}

bool Value::isNull() const
{
    return type() == NullType;
}

bool Value::isUndefined() const
{
    return type() == UndefinedType;
}

bool Value::isBool() const
{
    return type() == BoolType;
}

bool Value::isPositiveInteger() const
{
    return type() == PositiveIntegerType;
}

bool Value::isNegativeInteger() const
{
    return type() == NegativeIntegerType;
}

bool Value::isDouble() const
{
    return type() == DoubleType;
}

bool Value::isString() const
{
    return type() == StringType;
}

bool Value::isByteString() const
{
    return type() == ByteStringType;
}

bool Value::isArray() const
{
    return type() == ArrayType;
}

bool Value::isMap() const
{
    return type() == MapType;
}

bool Value::isBigInteger() const
{
    return type() == BigIntegerType;
}

bool Value::toBool() const
{
    return castTo<bool>();
}

uint64_t Value::toPositiveInteger() const
{
    return castTo<PositiveInteger>().value;
}

uint64_t Value::toNegativeInteger() const
{
    return castTo<NegativeInteger>().value;
}

double Value::toDouble() const
{
    return castTo<double>();
}

std::string Value::toString() const
{
    return castTo<std::string>();
}

std::vector<char> Value::toByteString() const
{
    return castTo< std::vector<char> >();
}

std::vector<Value> Value::toArray() const
{
    return castTo< std::vector<Value> >();
}

std::map<Value, Value> Value::toMap() const
{
    return castTo< std::map<Value, Value> >();
}

Value::BigInteger Value::toBigInteger() const
{
    return castTo<BigInteger>();
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
    else if( isPositiveInteger() )
    {
        return boost::lexical_cast<std::string>(toPositiveInteger());
    }
    else if( isNegativeInteger() )
    {
        std::string result = "-";
        const char specialValue[] = "18446744073709551616"; // 0x10000000000000000
        uint64_t value = toNegativeInteger();

        if( value == 0 )
            result += specialValue;
        else
            result += boost::lexical_cast<std::string>(value);

        return result;
    }
    else if( isDouble() )
    {
        return boost::lexical_cast<std::string>(toDouble());
    }
    else if( isString() )
    {
        return toString();
    }
    else if( isByteString() )
    {
        std::vector<char> byteString = toByteString();
        std::string result = "(0x";

        static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                     'A', 'B', 'C', 'D', 'E', 'F'};

        for(size_t i = 0; i < byteString.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(byteString[i]);

            result += hex[c / sizeof(hex)];
            result += hex[c % sizeof(hex)];
        }

        result += ')';
        return result;
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
    else if( isBigInteger() )
    {
        BigInteger bigInteger = toBigInteger();
        std::string result;

        if( bigInteger.positive )
            result = "(big integer: 0x";
        else
            result = "(negative big integer: 0x";

        static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                     'A', 'B', 'C', 'D', 'E', 'F'};

        if( bigInteger.bigint.empty() == false )
        {
            for(size_t i = 0; i < bigInteger.bigint.size(); ++i)
            {
                unsigned char c = static_cast<unsigned char >(bigInteger.bigint[i]);

                result += hex[c / sizeof(hex)];
                result += hex[c % sizeof(hex)];
            }

            result.resize(result.size() - 1);
            result[result.size() - 1] = ')';
        }
        else
        {
            result += ')';
        }

        return result;
    }

    assert(false);
    std::string invalidType = "(invalid type)";
    return invalidType;
}

bool operator < (const Value &lhs, const Value &rhs)
{
    return lhs.value < rhs.value;
}

bool operator == (const Value &lhs, const Value &rhs)
{
    return lhs.value == rhs.value;
}
