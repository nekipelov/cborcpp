/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <ostream>

#include <boost/variant.hpp>

#include <stdint.h>

class Value {
public:
    enum Type {
        NullType,
        UndefinedType,
        BoolType,
        PositiveIntegerType,
        NegativeIntegerType,
        DoubleType,
        StringType,
        ByteStringType,
        ArrayType,
        MapType,
        BigIntegerType
    };

    struct NullTag {
        bool operator == (const NullTag &) const {
            return true;
        }

        bool operator < (const NullTag &) const {
            return true;
        }
    };

    struct UndefinedTag {
        bool operator == (const UndefinedTag &) const {
            return true;
        }

        bool operator < (const UndefinedTag &) const {
            return true;
        }
    };

    struct BigInteger {
        bool positive;
        std::vector<char> bigint; // Warning: big-endian byte order.

        bool operator == (const BigInteger &other) const {
            return positive == other.positive && bigint == other.bigint;
        }

        bool operator < (const BigInteger &other) const {
            if( positive == other.positive )
            {
                return bigint < other.bigint;
            }
            else
            {
                return positive < other.positive;
            }
        }
    };

    Value();
    Value(NullTag);
    Value(UndefinedTag);
    Value(bool v);
    Value(int i);
    Value(int64_t i);
    Value(uint64_t i, bool positive = true);
    Value(double d);
    Value(const std::string &s);
    Value(const std::vector<char> &bs);
    Value(const char *s);
    Value(const std::vector<Value> &vec);
    Value(const std::map<Value, Value> &map);
    Value(const BigInteger &bigint);

    static Value null();
    static Value undefiend();

    bool isNull() const;
    bool isUndefined() const;
    bool isBool() const;
    bool isPositiveInteger() const;
    bool isNegativeInteger() const;
    bool isDouble() const;
    bool isString() const;
    bool isByteString() const;
    bool isArray() const;
    bool isMap() const;
    bool isBigInteger() const;

    bool toBool() const;
    uint64_t toPositiveInteger() const;
    uint64_t toNegativeInteger() const;
    double toDouble() const;
    std::string toString() const;
    std::vector<char> toByteString() const;
    std::vector<Value> toArray() const;
    std::map<Value, Value> toMap() const;
    BigInteger toBigInteger() const;

    Type type() const;
    std::string inspect() const;

    template<typename T>
    static Value convertFrom(const std::vector<T> &arr);

    template<typename T>
    static Value convertFrom(const std::list<T> &list);

    template<typename TKey, typename TValue>
    static Value convertFrom(const std::map<TKey, TValue> &map);

protected:
    template<typename T>
    T castTo() const;

    template<typename T>
    bool typeEq() const;

private:

    struct PositiveInteger
    {
        bool operator == (const PositiveInteger &other) const {
            return value == other.value;
        }

        bool operator < (const PositiveInteger &other) const {
            return value < other.value;
        }

        uint64_t value;
    };

    struct NegativeInteger
    {
        bool operator == (const NegativeInteger &other) const {
            return value == other.value;
        }

        bool operator < (const NegativeInteger &other) const {
            return value < other.value;
        }
        uint64_t value;
    };

    boost::variant<NullTag, UndefinedTag, bool, PositiveInteger, NegativeInteger, double, std::string, std::vector<char>,
                   std::vector<Value>, std::map<Value, Value>, BigInteger > value;

    friend bool operator < (const Value &lhs, const Value &rhs);
    friend bool operator == (const Value &lhs, const Value &rhs);
};

bool operator < (const Value &lhs, const Value &rhs);
bool operator == (const Value &lhs, const Value &rhs);


template<typename T>
T Value::castTo() const
{
    if( value.type() == typeid(T) )
        return boost::get<T>(value);
    else
        return T();
}

template<typename T>
bool Value::typeEq() const
{
    if( value.type() == typeid(T) )
        return true;
    else
        return false;
}

template<typename T>
Value Value::convertFrom(const std::vector<T> &arr)
{
    std::vector<Value> result;

    result.reserve(arr.size());
    std::copy(arr.begin(), arr.end(), std::back_inserter(result));
    return result;
}

template<typename T>
Value Value::convertFrom(const std::list<T> &list)
{
    std::vector<Value> result;
    std::copy(list.begin(), list.end(), std::back_inserter(result));
    return result;
}

template<typename TKey, typename TValue>
Value Value::convertFrom(const std::map<TKey, TValue> &map)
{
    std::map<Value, Value> result;
    std::copy(map.begin(), map.end(), std::inserter(result, result.end()));
    return result;
}

#endif // VALUE_H
