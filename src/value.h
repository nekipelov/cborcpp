/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <vector>
#include <map>

#include <boost/variant.hpp>

#include <stdint.h>

class Value {
public:
    enum Type {
        Null,
        Undefined,
        Bool,
        Integer,
        Double,
        String,
        Array,
        Map
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

    Value();
    Value(NullTag);
    Value(UndefinedTag);
    Value(bool v);
    Value(int32_t i);
    Value(uint32_t i);
    Value(int64_t i);
    Value(uint64_t i);
    Value(double d);
    Value(const std::string &s);
    Value(const char *s);
    Value(const std::vector<Value> &vec);
    Value(const std::map<Value, Value> &map);

    static Value null();
    static Value undefiend();

    bool isNull() const;
    bool isUndefined() const;
    bool isBool() const;
    bool isInt() const;
    bool isDouble() const;
    bool isString() const;
    bool isArray() const;
    bool isMap() const;

    bool toBool() const;
    int toInt() const;
    int64_t toInt64() const;
    double toDouble() const;
    std::string toString() const;
    std::vector<Value> toArray() const;
    std::map<Value, Value> toMap() const;

    Type type() const;
    std::string inspect() const;

protected:
    template<typename T>
    T castTo() const;

    template<typename T>
    bool typeEq() const;

private:
    boost::variant<NullTag, UndefinedTag, bool, int64_t, double, std::string, std::vector<Value>,
                   std::map<Value, Value> > value;

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


#endif // VALUE_H
