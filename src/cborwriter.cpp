/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

// See http://tools.ietf.org/search/rfc7049

#include <string.h>
#include <endian.h>
#include <stdint.h>
#include <iostream>

#include "cborprivate.h"
#include "cborwriter.h"

static const int positiveIntegerStart = 0x00;
static const int negativeIntegerStart = 0x20;
static const int byteStringStart = 0x40;
static const int utf8StringStart = 0x60;
static const int arrayStart = 0x80;
static const int mapStart = 0xa0;
static const int taggedStart = 0xc0;
static const int textBasedDateTime = taggedStart;      // 0xc0
static const int epochBasedDateTime = taggedStart + 1; // 0xc1
static const int positiveBignum = taggedStart + 2;     // 0xc2
static const int negativeBignum = taggedStart + 3;     // 0xc3
//...
static const int simpleStart = 0xe0;
static const int halfPrecisionFloat = simpleStart + 0x19;   // 0xf9
static const int singlePrecisionFloat = simpleStart + 0x1a; // 0xfa
static const int doublePrecisionFloat = simpleStart + 0x1b; // 0xfb

static void cborWriteInternal(std::vector<char> &buff, const CborValue &value);

static void writeNull(std::vector<char> &buff)
{
    const char nil = static_cast<char>(0xf6);
    buff.push_back(nil);
}

static void writeUndefined(std::vector<char> &buff)
{
    const char undefined = static_cast<char>(0xf7);
    buff.push_back(undefined);
}

static void writeBool(std::vector<char> &buff, const CborValue &value)
{
    const char trueValue = static_cast<char>(0xf5);
    const char falseValue = static_cast<char>(0xf4);

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

static void writePositiveInteger(std::vector<char> &buff, const CborValue &value)
{
    uint64_t i = value.toPositiveInteger();

    if( i > 0 )
    {
        writeInteger(buff, i, positiveIntegerStart);
    }
    else
    {
        const char zero = 0;
        buff.push_back(zero);
    }
}

static void writeNegativeInteger(std::vector<char> &buff, const CborValue &value)
{
    uint64_t i = value.toNegativeInteger();

    if( i == 0 )
    {
        writeInteger(buff, 0xFFFFFFFFFFFFFFFF, negativeIntegerStart);
    }
    else
    {
        writeInteger(buff, i - 1, negativeIntegerStart);
    }
}

static void writeString(std::vector<char> &buff, const CborValue &value)
{
    const std::string &s = value.toString();

    writeInteger(buff, s.size(), utf8StringStart);

    buff.insert(buff.end(), s.begin(), s.end());
}

static void writeByteString(std::vector<char> &buff, const CborValue &value)
{
    const std::vector<char> &data = value.toByteString();

    writeInteger(buff, data.size(), byteStringStart);

    buff.insert(buff.end(), data.begin(), data.end());
}


static void writeDouble(std::vector<char> &buff, const CborValue &value)
{
    double dv = value.toDouble();
    float fv = dv;

    if( fv == dv )
    {
        // Warning: code from MessagePack

        union {
            float f;
            uint32_t ui32;
        } buf = { fv };

        int i32 = buf.ui32;
        if ((i32 & 0x1FFF) == 0)
        {
            do
            {
                // IEEE 754 half-precision
                uint16_t s16 = (i32 >> 16) & 0x8000;
                int exponent = (i32 >> 23) & 0xff;
                int mantissa = i32 & 0x7fffff;

                if (exponent == 0 && mantissa == 0)
                {
                    ;
                }
                else if (exponent >= 113 && exponent <= 142)
                {
                    // normalized
                    s16 += ((exponent - 112) << 10) + (mantissa >> 13);
                }
                else if (exponent >= 103 && exponent < 113)
                {
                    // denorm, exp16 = 0
                    if (mantissa & ((1 << (126 - exponent)) - 1))
                        break; // loss of precision
                    s16 += ((mantissa + 0x800000) >> (126 - exponent));
                }
                else if (exponent == 255 && mantissa == 0)
                {
                    // Inf
                    s16 += 0x7c00;
                }
                else
                {
                    break;  // loss of range
                }

                s16 = htobe16(s16);

                uint8_t bytes[3] = {
                    halfPrecisionFloat,
                    static_cast<uint8_t>(s16 & 0xff),
                    static_cast<uint8_t>((s16 & 0xff00) >> 8)
                };

                buff.insert(buff.end(), bytes, bytes + sizeof(bytes));
                return;
            } while(0);
        }


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

static void writeArray(std::vector<char> &buff, const CborValue &value)
{
    const std::vector<CborValue> &arr = value.toArray();

    writeInteger(buff, arr.size(), arrayStart);

    for(size_t i = 0; i < arr.size(); ++i)
    {
        cborWriteInternal(buff, arr[i]);
    }
}

static void writeMap(std::vector<char> &buff, const CborValue &value)
{
    const std::map<CborValue, CborValue> &map = value.toMap();
    std::map<CborValue, CborValue>::const_iterator it = map.begin();
    std::map<CborValue, CborValue>::const_iterator end = map.end();

    writeInteger(buff, map.size(), mapStart);

    for(; it != end; ++it)
    {
        cborWriteInternal(buff, it->first);
        cborWriteInternal(buff, it->second);
    }
}

static void writeBigInteger(std::vector<char> &buff, const CborValue &value)
{
    CborValue::BigInteger bigInteger = value.toBigInteger();

    bool canBeWrittenAs64BitInteger = bigInteger.bigint.size() < 9;
    canBeWrittenAs64BitInteger = canBeWrittenAs64BitInteger || (
                                    bigInteger.bigint.size() == 9 &&
                                    bigInteger.bigint[8] == 0 &&
                                    bigInteger.bigint[7] == 0 &&
                                    bigInteger.bigint[6] == 0 &&
                                    bigInteger.bigint[5] == 0 &&
                                    bigInteger.bigint[4] == 0 &&
                                    bigInteger.bigint[3] == 0 &&
                                    bigInteger.bigint[2] == 0 &&
                                    bigInteger.bigint[1] == 0 &&
                                    bigInteger.bigint[0] == 1 &&
                                    bigInteger.positive == false);

    if(canBeWrittenAs64BitInteger)
    {
        // This number can be written as integer value.

        if( bigInteger.bigint.size() == 9 )
        {
            writeInteger(buff, 0xFFFFFFFFFFFFFFFF, negativeIntegerStart);
        }
        else
        {
            uint64_t ui = 0;

            for(size_t i = 0; i < bigInteger.bigint.size(); ++i)
                ui = bigInteger.bigint[i] + (ui << 8);

            if( bigInteger.positive)
                writeInteger(buff, ui, positiveIntegerStart);
            else
                writeInteger(buff, ui - 1, negativeIntegerStart);
        }
    }
    else
    {
        if( bigInteger.positive )
        {
            buff.push_back(static_cast<char>(positiveBignum));
        }
        else
        {
            buff.push_back(static_cast<char>(negativeBignum));

            for(size_t i = bigInteger.bigint.size(); i != 0 ; --i)
            {
                unsigned char c = bigInteger.bigint[i - 1];
                if( c != 0 )
                {
                    bigInteger.bigint[i - 1] = c - 1u;
                    break;
                }
                else
                {
                    bigInteger.bigint[i - 1] = static_cast<char>(0xff);
                }
            }
        }

        writeByteString(buff, CborValue(bigInteger.bigint));
    }
}

static void cborWriteInternal(std::vector<char> &buff, const CborValue &value)
{
    switch(value.type())
    {
    case CborValue::NullType:
        writeNull(buff);
        break;
    case CborValue::UndefinedType:
        writeUndefined(buff);
        break;
    case CborValue::BoolType:
        writeBool(buff, value);
        break;
    case CborValue::NegativeIntegerType:
        writeNegativeInteger(buff, value);
        break;
    case CborValue::PositiveIntegerType:
        writePositiveInteger(buff, value);
        break;
    case CborValue::DoubleType:
        writeDouble(buff, value);
        break;
    case CborValue::StringType:
        writeString(buff, value);
        break;
    case CborValue::ByteStringType:
        writeByteString(buff, value);
        break;
    case CborValue::ArrayType:
        writeArray(buff, value);
        break;
    case CborValue::MapType:
        writeMap(buff, value);
        break;
    case CborValue::BigIntegerType:
        writeBigInteger(buff, value);
        break;
    default:
        assert(false);
        std::cerr << "Internal error: invalid type" << std::endl;
    }
}

std::vector<char> cborWrite(const CborValue &value)
{
    std::vector<char> result;

    cborWriteInternal(result, value);
    return result;
}

