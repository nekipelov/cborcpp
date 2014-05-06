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
#include <stdexcept>

#include <stdint.h>

#include <boost/variant.hpp>
#include <boost/scoped_ptr.hpp>

class CborValue {
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

    class IteratorImpl;
    class Iterator {
    public:
        Iterator(const CborValue &value);
        ~Iterator();

        bool hasNext() const;
        bool hasPrev() const;

        CborValue next();
        CborValue prev();

        // Index or map key
        CborValue key() const;
        // Value
        CborValue value() const;

    private:
        friend class CborValue;
        boost::scoped_ptr<IteratorImpl> pimpl;
    };

    CborValue();
    CborValue(NullTag);
    CborValue(UndefinedTag);
    CborValue(bool v);
    CborValue(int i);
    CborValue(int64_t i);
    CborValue(uint64_t i, bool positive = true);
    CborValue(double d);
    CborValue(const std::string &s);
    CborValue(const std::vector<char> &bs);
    CborValue(const char *s);
    CborValue(const std::vector<CborValue> &vec);
    CborValue(const std::map<CborValue, CborValue> &map);
    CborValue(const BigInteger &bigint);

    static CborValue null();
    static CborValue undefiend();

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
    std::vector<CborValue> toArray() const;
    std::map<CborValue, CborValue> toMap() const;
    BigInteger toBigInteger() const;

    Type type() const;
    std::string inspect() const;

    // map and array
    size_t size() const;
    bool isEmpty() const;

    // for map
    bool hasMember(const CborValue &key) const;
    CborValue member(const CborValue &key) const;

    bool hasMember(const char *key) const;
    CborValue member(const char *key) const;

    template<typename T>
    bool hasMember(const T &key) const;
    template<typename T>
    CborValue member(const T &key) const;

    // for array
    CborValue at(size_t arrayIndex) const;

    template<typename T>
    static CborValue convertFrom(const std::vector<T> &arr);

    template<typename T>
    static CborValue convertFrom(const std::list<T> &list);

    template<typename TKey, typename TValue>
    static CborValue convertFrom(const std::map<TKey, TValue> &map);

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

    typedef boost::variant<NullTag, UndefinedTag, bool, PositiveInteger, NegativeInteger,
                           double, std::string, std::vector<char>, std::vector<CborValue>,
                           std::map<CborValue, CborValue>, BigInteger > Variant;

    Variant value;

    friend bool operator < (const CborValue &lhs, const CborValue &rhs);
    friend bool operator == (const CborValue &lhs, const CborValue &rhs);
};

bool operator < (const CborValue &lhs, const CborValue &rhs);
bool operator == (const CborValue &lhs, const CborValue &rhs);


inline bool CborValue::hasMember(const char *key) const
{
    return hasMember(std::string(key));
}

inline CborValue CborValue::member(const char *key) const
{
    return member(std::string(key));
}

template<typename T>
bool CborValue::hasMember(const T &key) const
{
    return hasMember(CborValue(key));
}

template<typename T>
CborValue CborValue::member(const T &key) const
{
    return member(CborValue(key));
}

template<typename T>
T CborValue::castTo() const
{
    if( value.type() == typeid(T) )
        return boost::get<T>(value);
    // else
    //    return T();

    throw std::runtime_error( "CborValue: cast error");
}

template<typename T>
bool CborValue::typeEq() const
{
    if( value.type() == typeid(T) )
        return true;
    else
        return false;
}

template<typename T>
CborValue CborValue::convertFrom(const std::vector<T> &arr)
{
    std::vector<CborValue> result;

    result.reserve(arr.size());
    std::copy(arr.begin(), arr.end(), std::back_inserter(result));
    return result;
}

template<typename T>
CborValue CborValue::convertFrom(const std::list<T> &list)
{
    std::vector<CborValue> result;
    std::copy(list.begin(), list.end(), std::back_inserter(result));
    return result;
}

template<typename TKey, typename TValue>
CborValue CborValue::convertFrom(const std::map<TKey, TValue> &map)
{
    std::map<CborValue, CborValue> result;
    std::copy(map.begin(), map.end(), std::inserter(result, result.end()));
    return result;
}

#endif // VALUE_H
