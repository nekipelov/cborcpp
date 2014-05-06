/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#include <limits>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "cborvalue.h"

struct ValueSizeVisitor : public boost::static_visitor<size_t>
{
    size_t operator()(const std::vector<CborValue> &arr) const
    {
        return arr.size();
    }
    size_t operator()(const std::map<CborValue, CborValue> &map) const
    {
        return map.size();
    }

    template<typename T>
    size_t operator()(const T &) const
    {
        //return 0;
        throw std::runtime_error( "CborValue: invalid type");
    }
};

struct ValueHasMemberVisitor : public boost::static_visitor<bool>
{
    ValueHasMemberVisitor(const CborValue &key)
        : key(key)
    {}

    bool operator()(const std::map<CborValue, CborValue> &map) const
    {
        return map.find(key) != map.end();
    }

    template<typename T>
    bool operator()(const T &) const
    {
        //return false;
        throw std::runtime_error( "CborValue: invalid type");
    }

    const CborValue &key;
};

struct ValueGetMemberVisitor : public boost::static_visitor<CborValue>
{
    ValueGetMemberVisitor(const CborValue &key)
        : key(key)
    {}

    CborValue operator()(const std::map<CborValue, CborValue> &map) const
    {
        std::map<CborValue, CborValue>::const_iterator it = map.find(key);

        if( it != map.end() )
            return it->second;
//        else
//            return CborValue::null();

        throw std::runtime_error( "CborValue: invalid type");
    }

    template<typename T>
    CborValue operator()(const T &) const
    {
        //return CborValue::null();
        throw std::runtime_error( "CborValue: invalid type");
    }

    const CborValue &key;
};

struct ValueGetArrayItemVisitor : public boost::static_visitor<CborValue>
{
    ValueGetArrayItemVisitor(size_t index)
        : index(index)
    {}

    CborValue operator()(const std::vector<CborValue> &arr) const
    {
        if( index < arr.size() )
            return arr[index];
        else
            return CborValue::null();
    }

    template<typename T>
    CborValue operator()(const T &) const
    {
        return CborValue::null();
    }

    const size_t index;
};

CborValue::CborValue()
    : value(NullTag())
{
}

CborValue::CborValue(NullTag)
    :  value(NullTag())
{
}

CborValue::CborValue(UndefinedTag)
    :  value(UndefinedTag())
{
}


CborValue::CborValue(bool b)
    : value(b)
{
}

CborValue::CborValue(int i)
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

CborValue::CborValue(int64_t i)
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

CborValue::CborValue(uint64_t i, bool positive)
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

CborValue::CborValue(double d)
    : value(d)
{
}

CborValue::CborValue(const std::string &s)
    : value(s)
{
}

CborValue::CborValue(const std::vector<char> &bs)
    : value(bs)
{
}

CborValue::CborValue(const char *s)
    : value(std::string(s))
{
}

CborValue::CborValue(const std::vector<CborValue> &vec)
    : value(vec)
{
}

CborValue::CborValue(const std::map<CborValue, CborValue> &map)
    : value(map)
{
}

CborValue::CborValue(const BigInteger &bigint)
    : value(bigint)
{
}

CborValue CborValue::null()
{
    return CborValue(NullTag());
}

CborValue CborValue::undefiend()
{
    return CborValue(UndefinedTag());
}

bool CborValue::isNull() const
{
    return type() == NullType;
}

bool CborValue::isUndefined() const
{
    return type() == UndefinedType;
}

bool CborValue::isBool() const
{
    return type() == BoolType;
}

bool CborValue::isPositiveInteger() const
{
    return type() == PositiveIntegerType;
}

bool CborValue::isNegativeInteger() const
{
    return type() == NegativeIntegerType;
}

bool CborValue::isDouble() const
{
    return type() == DoubleType;
}

bool CborValue::isString() const
{
    return type() == StringType;
}

bool CborValue::isByteString() const
{
    return type() == ByteStringType;
}

bool CborValue::isArray() const
{
    return type() == ArrayType;
}

bool CborValue::isMap() const
{
    return type() == MapType;
}

bool CborValue::isBigInteger() const
{
    return type() == BigIntegerType;
}

bool CborValue::toBool() const
{
    return castTo<bool>();
}

uint64_t CborValue::toPositiveInteger() const
{
    return castTo<PositiveInteger>().value;
}

uint64_t CborValue::toNegativeInteger() const
{
    return castTo<NegativeInteger>().value;
}

double CborValue::toDouble() const
{
    return castTo<double>();
}

std::string CborValue::toString() const
{
    return castTo<std::string>();
}

std::vector<char> CborValue::toByteString() const
{
    return castTo< std::vector<char> >();
}

std::vector<CborValue> CborValue::toArray() const
{
    return castTo< std::vector<CborValue> >();
}

std::map<CborValue, CborValue> CborValue::toMap() const
{
    return castTo< std::map<CborValue, CborValue> >();
}

CborValue::BigInteger CborValue::toBigInteger() const
{
    return castTo<BigInteger>();
}

CborValue::Type CborValue::type() const
{
    return static_cast<CborValue::Type>(value.which());
}

std::string CborValue::inspect() const
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
        std::vector<CborValue> values = toArray();
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
        std::map<CborValue, CborValue> values = toMap();
        std::string result = "{";

        if( values.empty() == false )
        {
            std::map<CborValue, CborValue>::iterator it = values.begin();
            std::map<CborValue, CborValue>::iterator end = values.end();

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

size_t CborValue::size() const
{
    return boost::apply_visitor(ValueSizeVisitor(), value);
}

bool CborValue::isEmpty() const
{
    return size() == 0;
}

bool CborValue::hasMember(const CborValue &key) const
{
    return boost::apply_visitor(ValueHasMemberVisitor(key), value);
}

CborValue CborValue::member(const CborValue &key) const
{
    return boost::apply_visitor(ValueGetMemberVisitor(key), value);
}

CborValue CborValue::at(size_t arrayIndex) const
{
    return boost::apply_visitor(ValueGetArrayItemVisitor(arrayIndex), value);
}

bool operator < (const CborValue &lhs, const CborValue &rhs)
{
    return lhs.value < rhs.value;
}

bool operator == (const CborValue &lhs, const CborValue &rhs)
{
    return lhs.value == rhs.value;
}

class CborValue::IteratorImpl
{
public:
    IteratorImpl(const std::map<CborValue, CborValue> &container)
        : type(Map), map(container), mapIteratorPos(container.begin()),
          mapIteratorVal(container.end())
    {
    }

    IteratorImpl(const std::vector<CborValue> &container)
        : type(Array), array(container), arrayIteratorPos(container.begin()),
          arrayIteratorVal(container.end())
    {
    }

    enum {
        Invalid,
        Array,
        Map
    } type;

    boost::optional<const std::map<CborValue, CborValue> &> map;
    boost::optional<const std::vector<CborValue> &> array;

    std::map<CborValue, CborValue>::const_iterator mapIteratorPos;
    std::map<CborValue, CborValue>::const_iterator mapIteratorVal;
    std::vector<CborValue>::const_iterator arrayIteratorPos;
    std::vector<CborValue>::const_iterator arrayIteratorVal;
};


CborValue::Iterator::Iterator(const CborValue &value)
{
    switch(value.type())
    {
    case CborValue::ArrayType:
        pimpl.reset(new IteratorImpl(boost::get< std::vector<CborValue> >(value.value)));
        break;
    case CborValue::MapType:
        pimpl.reset(new IteratorImpl(boost::get< std::map<CborValue, CborValue> >(value.value)));
        break;
    default:
        break;
    }
}

CborValue::Iterator::~Iterator()
{
}

bool CborValue::Iterator::hasNext() const
{
    if( pimpl->type == CborValue::IteratorImpl::Array )
    {
        return pimpl->arrayIteratorPos != pimpl->array->end();
    }
    else if( pimpl->type == CborValue::IteratorImpl::Map )
    {
        return pimpl->mapIteratorPos != pimpl->map->end();
    }
    else
    {
        return false;
    }
}

bool CborValue::Iterator::hasPrev() const
{
    if( pimpl->type == CborValue::IteratorImpl::Array )
        return pimpl->arrayIteratorPos != pimpl->array->begin();
    else if( pimpl->type == CborValue::IteratorImpl::Map )
        return pimpl->mapIteratorPos != pimpl->map->begin();
    else
        return false;
}

CborValue CborValue::Iterator::next()
{
    if( hasNext() )
    {
        if( pimpl->type == CborValue::IteratorImpl::Array )
        {
            if( pimpl->arrayIteratorPos != pimpl->array->end() )
            {
                pimpl->arrayIteratorVal = pimpl->arrayIteratorPos++;
                return *(pimpl->arrayIteratorVal);
            }
            else
            {
                return CborValue::null();
            }
        }
        else if( pimpl->type == CborValue::IteratorImpl::Map )
        {
            if( pimpl->mapIteratorPos != pimpl->map->end() )
            {
                pimpl->mapIteratorVal = pimpl->mapIteratorPos++;
                return pimpl->mapIteratorVal->second;
            }
            else
            {
                return CborValue::null();
            }
        }
    }

    return CborValue::null();
}

CborValue CborValue::Iterator::prev()
{
    if( pimpl->type == CborValue::IteratorImpl::Array )
    {
        if( pimpl->arrayIteratorPos != pimpl->array->begin() )
        {
            pimpl->arrayIteratorVal = pimpl->arrayIteratorPos--;
            return *(pimpl->arrayIteratorVal);
        }
        else
        {
            return CborValue::null();
        }
    }
    else if( pimpl->type == CborValue::IteratorImpl::Map )
    {
        if( pimpl->mapIteratorPos != pimpl->map->begin() )
        {
            pimpl->mapIteratorVal = pimpl->mapIteratorPos--;
            return pimpl->mapIteratorVal->second;
        }
        else
        {
            return CborValue::null();
        }
    }
    else
    {
        return CborValue::null();
    }
}

CborValue CborValue::Iterator::key() const
{
    if( pimpl->type == CborValue::IteratorImpl::Array )
    {
        if( pimpl->arrayIteratorVal != pimpl->array->end() )
        {
            return CborValue(std::distance(pimpl->array->begin(), pimpl->arrayIteratorVal));
        }
        else
        {
            return CborValue::null();
        }
    }
    else if( pimpl->type == CborValue::IteratorImpl::Map )
    {
        if( pimpl->mapIteratorVal != pimpl->map->end() )
        {
            return pimpl->mapIteratorVal->first;
        }
        else
        {
            return CborValue::null();
        }
    }
    else
    {
        return CborValue::null();
    }
}

CborValue CborValue::Iterator::value() const
{
    if( pimpl->type == CborValue::IteratorImpl::Array )
    {
        if( pimpl->arrayIteratorVal != pimpl->array->end() )
        {
            return *pimpl->arrayIteratorVal;
        }
        else
        {
            return CborValue::null();
        }
    }
    else if( pimpl->type == CborValue::IteratorImpl::Map )
    {
        if( pimpl->mapIteratorVal != pimpl->map->end() )
        {
            return pimpl->mapIteratorVal->second;
        }
        else
        {
            return CborValue::null();
        }
    }
    else
    {
        return CborValue::null();
    }
}
