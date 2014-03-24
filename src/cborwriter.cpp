/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

// See http://tools.ietf.org/search/rfc7049

#include <string.h>
#include <endian.h>
#include <stdint.h>

#include "cborprivate.h"
#include "cborwriter.h"

static const int negativeIntegerStart = 0x20;
static const int byteStringStart = 0x40;
static const int utf8StringStart = 0x60;
static const int arrayStart = 0x80;
static const int mapStart = 0xa0;
static const int simpleStart = 0xe0;
static const int halfPrecisionFloat = simpleStart + 0x19;   // 0xf9
static const int singlePrecisionFloat = simpleStart + 0x1a; // 0xfa
static const int doublePrecisionFloat = simpleStart + 0x1b; // 0xfb

static void cborWriteInternal(std::vector<char> &buff, const Value &value);

static void writeNull(std::vector<char> &buff)
{
    const char nil = 0xf6;
    buff.push_back(nil);
}

static void writeUndefined(std::vector<char> &buff)
{
    const char undefined = 0xf7;
    buff.push_back(undefined);
}

static void writeBool(std::vector<char> &buff, const Value &value)
{
    const char trueValue = 0xf5;
    const char falseValue = 0xf4;

    if( value.toBool() )
        buff.push_back(trueValue);
    else
        buff.push_back(falseValue);
}

static void writeInteger(std::vector<char> &buff, uint64_t value, int type)
{
    assert(type + 24 < 256);

    if( value < 24 )
    {
        uint8_t byte = type + value;

        buff.push_back(byte);
    }
    else if( value < 256 )
    {
        uint8_t bytes[2] = {static_cast<uint8_t>(type + 24), static_cast<uint8_t>(value)};

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
    else if( value < 65536 )
    {
        uint16_t ui16 = htobe16(value);
        uint8_t bytes[3] = {
            static_cast<uint8_t>(type + 25),
            static_cast<uint8_t>(ui16 & 0xff),
            static_cast<uint8_t>((ui16 & 0xff00) >> 8)
        };

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
    else if( value < 4294967296LU )
    {
        uint32_t ui32 = htobe32(value);
        uint8_t bytes[5] = {
            static_cast<uint8_t>(type + 26),
            static_cast<uint8_t>(ui32 & 0xff),
            static_cast<uint8_t>((ui32 & 0xff00) >> 8),
            static_cast<uint8_t>((ui32 & 0xff0000) >> 16),
            static_cast<uint8_t>((ui32 & 0xff000000) >> 24)
        };

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
    else
    {
        uint64_t ui64 = htobe64(value);
        uint8_t bytes[9] = {
            static_cast<uint8_t>(type + 27),
            static_cast<uint8_t>(ui64 & 0xff),
            static_cast<uint8_t>((ui64 & 0xff00) >> 8),
            static_cast<uint8_t>((ui64 & 0xff0000) >> 16),
            static_cast<uint8_t>((ui64 & 0xff000000) >> 24),
            static_cast<uint8_t>((ui64 & 0xff00000000UL) >> 32),
            static_cast<uint8_t>((ui64 & 0xff0000000000UL) >> 40),
            static_cast<uint8_t>((ui64 & 0xff000000000000UL) >> 48),
            static_cast<uint8_t>((ui64 & 0xff00000000000000UL) >> 56)
        };

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
}

static void writeInteger(std::vector<char> &buff, const Value &value)
{
    int64_t i = value.toInt64();

    if( i > 0 )
    {
        writeInteger(buff, i, false);
    }
    else if( i < 0 )
    {
        uint64_t ui = i ^ (i >> 63);
        writeInteger(buff, ui, negativeIntegerStart);
    }
    else
    {
        const char zero = 0;
        buff.push_back(zero);
    }
}

static void writeString(std::vector<char> &buff, const Value &value)
{
    const std::string &s = value.toString();

    writeInteger(buff, s.size(), utf8StringStart);

    buff.insert(buff.end(), s.begin(), s.end());
}

static void writeDouble(std::vector<char> &buff, const Value &value)
{
    double dv = value.toDouble();
    float fv = dv;

    if( fv == dv )
    {
        union {
            float f;
            uint32_t ui32;
        } buf = { fv };

        int i32 = buf.ui32;
        if ((i32 & 0x1FFF) == 0)
        {
            // IEEE 754 half-precision
            uint16_t s16 = (i32 >> 16) & 0x8000;
            int exponent = (i32 >> 23) & 0xff;
            int mantissa = i32 & 0x7fffff;

            if (exponent == 0 && mantissa == 0)
            {
                ;
            }
            else if (exponent >= 113 && exponent <= 142) /* normalized */
            {
                s16 += ((exponent - 112) << 10) + (mantissa >> 13);
            }
            else if (exponent >= 103 && exponent < 113)
            {
                /* denorm, exp16 = 0 */
                if (mantissa & ((1 << (126 - exponent)) - 1))
                    goto float32;         /* loss of precision */
                s16 += ((mantissa + 0x800000) >> (126 - exponent));
            }
            else if (exponent == 255 && mantissa == 0)
            {
                /* Inf */
                s16 += 0x7c00;
            }
            else
            {
                goto float32;           /* loss of range */
            }

            s16 = htobe16(s16);

            uint8_t bytes[3] = {
                halfPrecisionFloat,
                static_cast<uint8_t>(s16 & 0xff),
                static_cast<uint8_t>((s16 & 0xff00) >> 8)
            };

            buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
            return;
        }

float32:
        // IEEE 754 single-precision
        buf.ui32 = htobe32(buf.ui32);

        uint8_t bytes[5] = {
            singlePrecisionFloat,
            static_cast<uint8_t>(buf.ui32 & 0xff),
            static_cast<uint8_t>((buf.ui32 & 0xff00) >> 8),
            static_cast<uint8_t>((buf.ui32 & 0xff0000) >> 16),
            static_cast<uint8_t>((buf.ui32 & 0xff000000) >> 24),
        };

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
    else if( dv != dv )
    {
        // NaN
        uint8_t bytes[3] = {halfPrecisionFloat, 0x7e, 0x00};

        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
    else
    {
        // IEEE 754 double-precision
        union {
            double dv;
            uint64_t ui64;
        } buf = { dv };

        buf.ui64 = htobe64(buf.ui64);

        uint8_t bytes[9] = {
            doublePrecisionFloat,
            static_cast<uint8_t>(buf.ui64 & 0xff),
            static_cast<uint8_t>((buf.ui64 & 0xff00) >> 8),
            static_cast<uint8_t>((buf.ui64 & 0xff0000) >> 16),
            static_cast<uint8_t>((buf.ui64 & 0xff000000) >> 24),
            static_cast<uint8_t>((buf.ui64 & 0xff00000000UL) >> 32),
            static_cast<uint8_t>((buf.ui64 & 0xff0000000000UL) >> 40),
            static_cast<uint8_t>((buf.ui64 & 0xff000000000000UL) >> 48),
            static_cast<uint8_t>((buf.ui64 & 0xff00000000000000UL) >> 56)
        };


        buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
    }
}

static void writeArray(std::vector<char> &buff, const Value &value)
{
    const std::vector<Value> &arr = value.toArray();

    writeInteger(buff, arr.size(), arrayStart);

    for(size_t i = 0; i < arr.size(); ++i)
    {
        cborWriteInternal(buff, arr[i]);
    }
}

static void writeMap(std::vector<char> &buff, const Value &value)
{
    const std::map<Value, Value> &map = value.toMap();
    std::map<Value, Value>::const_iterator it = map.begin();
    std::map<Value, Value>::const_iterator end = map.end();

    writeInteger(buff, map.size(), mapStart);

    for(; it != end; ++it)
    {
        cborWriteInternal(buff, it->first);
        cborWriteInternal(buff, it->second);
    }
}

static void cborWriteInternal(std::vector<char> &buff, const Value &value)
{
    switch(value.type())
    {
    case Value::Null:
        writeNull(buff);
        break;
    case Value::Undefined:
        writeUndefined(buff);
        break;
    case Value::Bool:
        writeBool(buff, value);
        break;
    case Value::Integer:
        writeInteger(buff, value);
        break;
    case Value::Double:
        writeDouble(buff, value);
        break;
    case Value::String:
        writeString(buff, value);
        break;
    case Value::Array:
        writeArray(buff, value);
        break;
    case Value::Map:
        writeMap(buff, value);
        break;
    default:
        assert(false);
        std::cerr << "Internal error: invalid type" << std::endl;
    }
}

std::vector<char> cborWrite(const Value &value)
{
    std::vector<char> result;

    cborWriteInternal(result, value);
    return result;
}

