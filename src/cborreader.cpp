/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

// See http://tools.ietf.org/search/rfc7049

#include <limits>

#include <math.h>
#include <string.h>
#include <stdint.h>

#include "cborprivate.h"
#include "cborreader.h"

static std::pair<size_t, CborValue> internalRead(const unsigned char *s, size_t size);

std::pair<size_t, uint64_t> readIntegerValue(unsigned char minorType, const unsigned char *data, size_t size)
{
    uint64_t result = 0;
    size_t bytesCount = 0;

    switch(minorType)
    {
        case 0x18: {
            // one byte uint8_t follows
            if( size < 2 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, 0);
            }
            bytesCount = 2;
            result = data[1];
            break;
        }
        case 0x19: {
            // two byte uint16_t follows
            if( size < 3 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, 0);
            }

            uint16_t value = *reinterpret_cast<const uint16_t *>(&data[1]);
            value = be16toh(value);
            result = value;
            bytesCount = 3;
            break;
        }
        case 0x1a: {
            // four byte uint32_t follows
            if( size < 5 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, 0);
            }

            uint32_t value = *reinterpret_cast<const uint32_t *>(&data[1]);
            value = be32toh(value);
            result = value;
            bytesCount = 5;
            break;
        }
        case 0x1b: {
            // eight byte uint64_t follows
            if( size < 9 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, 0);
            }

            uint64_t value = *reinterpret_cast<const uint64_t *>(&data[1]);
            value = be64toh(value);
            result = value;
            bytesCount = 9;
            break;
        }

        default:
            if( minorType <= 0x17 )
            {
                result = static_cast<uint32_t>(minorType);
                bytesCount = 1;
            }
    }

    return std::make_pair(bytesCount, result);
}

std::pair<size_t, CborValue> readPositiveInteger(unsigned char minorType, const unsigned char *data, size_t size)
{

    return readIntegerValue(minorType, data, size);
}

std::pair<size_t, CborValue> readNegativeInteger(unsigned char minorType, const unsigned char *data, size_t size)
{
    std::pair<size_t, uint64_t> pair = readIntegerValue(minorType, data, size);

    if( pair.first != 0 )
    {
        uint64_t value = pair.second;
        if( value == 0xffffffffffffffff )
        {
            // 18446744073709551617
            const char bigNumData [] = "\x01\x00\x00\x00\x00\x00\x00\x00\x00";
            CborValue::BigInteger bigInteger;

            bigInteger.positive = false;
            bigInteger.bigint.assign(bigNumData, bigNumData + sizeof(bigNumData) - 1);

            return std::make_pair(pair.first, CborValue(bigInteger));

        }
        else
        {
            return std::make_pair(pair.first, CborValue(value + 1, false));
        }
    }
    else
    {
        return pair;
    }

}

std::pair<size_t, CborValue> simpleOrFloat(unsigned char minorType, const unsigned char *data, size_t size)
{
    switch (minorType) {
        case FalseValue:
            return std::make_pair(1, CborValue(false));
            break;
        case TrueValue:
            return std::make_pair(1, CborValue(true));
            break;
        case NullValue:
            return std::make_pair(1, CborValue(CborValue::NullTag()));
            break;
        case UndefiendValue:
            return std::make_pair(1, CborValue(CborValue::UndefinedTag()));
            break;
        case SimpleValue1Byte:
            // TODO
            abort();
            break;
        case HalfPrecisionFloat: {
            if( size < 3 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, CborValue());
            }

            // adapte from code in rfc7049, Appendix D.
            uint8_t high = static_cast<uint8_t>(data[1]);
            int exponent = (high >> 2) & 0x1f;
            int mantissa = ((high & 0x3) << 8) | static_cast<uint8_t>(data[2]);

            double value = 0;

            if (exponent == 0)
                value = ldexp(mantissa, -24);
            else if (exponent != 31)
                value = ldexp(mantissa + 1024, exponent - 25);
            else
                value = mantissa == 0 ? std::numeric_limits<double>::infinity() :
                                    std::numeric_limits<double>::quiet_NaN();

            if( high & 0x80 )
                value = -value;

            return std::make_pair(3, CborValue(value));
        }
        case SinglePrecisionFloat: {
            if( size < 5 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, CborValue());
            }

            union {
                uint32_t u32;
                float value;
            } buf;

            memcpy(&buf.u32, &data[1], sizeof(buf.u32));
            buf.u32 = be32toh(buf.u32);

            return std::make_pair(5, CborValue(buf.value));
        }
        case DoublePrecisionFloat: {
            if( size < 9 )
            {
                std::cerr << "Unexpected end of data" << std::endl;
                return std::make_pair(0, CborValue());
            }

            union {
                uint64_t u64;
                double value;
            } buf;

            memcpy(&buf.u64, &data[1], sizeof(buf.u64));
            buf.u64 = be64toh(buf.u64);

            return std::make_pair(9, CborValue(buf.value));
        }
    }

    std::cerr << "Internal error: invalid minor type " << static_cast<int>(minorType)
              << " for simple value" << std::endl;
    assert(false);

    return std::make_pair(0, CborValue());
}

std::pair<size_t, CborValue> readByteString(uint8_t minorType, const unsigned char *data, size_t size)
{
    if( minorType == 0x1f )  // todo: 0xff break string
        abort();

    std::pair<size_t, uint64_t> pair = readIntegerValue(minorType, data, size);
    size_t length = pair.second;

    if( pair.first == 0 )
    {
        // empty string?
        return std::make_pair(pair.first, CborValue(""));
    }

    if( length > pair.first + size )
    {
        std::cerr << "Unexpected end of data" << std::endl;
        return std::make_pair(0, CborValue());
    }

    const char *ptr = reinterpret_cast<const char *>(data + pair.first);
    std::vector<char> buf(ptr, ptr + length);

    return std::make_pair(pair.first + pair.second, buf);
}

std::pair<size_t, CborValue> readString(uint8_t minorType, const unsigned char *data, size_t size)
{
    if( minorType == 0x1f )  // todo: 0xff break string
        abort();

    std::pair<size_t, uint64_t> pair = readIntegerValue(minorType, data, size);
    size_t length = pair.second;

    if( pair.first == 0 )
    {
        // empty string?
        return std::make_pair(pair.first, CborValue(""));
    }

    if( length > pair.first + size )
    {
        std::cerr << "Unexpected end of data" << std::endl;
        return std::make_pair(0, CborValue());
    }

    const char *ptr = reinterpret_cast<const char *>(data + pair.first);

    return std::make_pair(pair.first + pair.second, std::string(ptr, ptr + length));
}

std::pair<size_t, CborValue> readArray(uint8_t minorType, const unsigned char *data, size_t size)
{
    if( minorType == 0x1f )  // todo: 0xff break array
        abort();

    std::pair<size_t, uint64_t> pair = readIntegerValue(minorType, data, size);
    size_t offset = pair.first;

    if( pair.first == 0 )
    {
        // empty array?
        return std::make_pair(pair.first, CborValue(std::vector<CborValue>()));
    }

    std::vector<CborValue> result;

    result.reserve(pair.second);

    for(size_t i = 0; i < pair.second; ++i)
    {
        if( offset >= size )
        {
            std::cerr << "Unexpected end of data" << std::endl;
            return std::make_pair(0, CborValue());
        }

        std::pair<size_t, CborValue> pair = internalRead(data + offset, size - offset);

        offset += pair.first;
        result.push_back(pair.second);
    }

    return std::make_pair(offset, result);
}

std::pair<size_t, CborValue> readMap(uint8_t minorType, const unsigned char *data, size_t size)
{
    if( minorType == 0x1f )  // todo: 0xff break array
        abort();

    std::pair<size_t, uint64_t> pair = readIntegerValue(minorType, data, size);
    size_t offset = pair.first;

    if( pair.first == 0 )
    {
        // empty map?
        return std::make_pair(pair.first, std::map<CborValue, CborValue>());
    }

    std::map<CborValue, CborValue> result;

    for(size_t i = 0; i < pair.second; ++i)
    {
        if( offset >= size )
        {
            std::cerr << "Unexpected end of data" << std::endl;
            return std::make_pair(0, CborValue());
        }

        std::pair<size_t, CborValue> pair1 = internalRead(data + offset, size - offset);
        std::pair<size_t, CborValue> pair2 = internalRead(data + offset + pair1.first, size - offset - pair1.first);

        assert( pair1.first != 0 );
        assert( pair2.first != 0 );

        offset += pair1.first;
        offset += pair2.first;

        result[pair1.second] = pair2.second;
    }

    return std::make_pair(offset, result);
}

std::pair<size_t, CborValue> readBignum(const unsigned char *data, size_t size, bool positive)
{
    CborValue::BigInteger bigInteger;
    std::pair<size_t, CborValue> pair = internalRead(data, size);

    if( pair.first == 0 )
    {
        // fail?
        return std::make_pair(pair.first, CborValue());
    }

    std::vector<char> binaryString = pair.second.toByteString();

    if( !positive )
    {
        for(size_t i = binaryString.size(); i != 0 ; --i)
        {
            unsigned char c = static_cast<unsigned char>(binaryString[i - 1]);
            if( c == 0xff )
            {
                binaryString[i - 1] = 0;
            }
            else
            {
                binaryString[i - 1] = c + 1u;
                break;
            }
        }
    }

    bigInteger.positive = positive;
    bigInteger.bigint.assign(binaryString.begin(), binaryString.end());


    return std::make_pair(pair.first, bigInteger);
}


std::pair<size_t, CborValue> readTagger(uint8_t minorType, const unsigned char *data, size_t size)
{
    switch(minorType)
    {
        case TextBasedDateTime:
            break;
        case EpochBasedDateTime:
            break;
        case PositiveBignum:
            return readBignum(data + 1, size - 1, true);
            break;
        case NegativeBignum:
            return readBignum(data + 1, size - 1, false);
            break;
        case DecimalFraction:
            break;
        case BigFloat:
            break;
    }

    std::cerr << "Internal error: invalid minor type for tagger value"
              << static_cast<int>(minorType) << std::endl;
    assert(false);

    return std::make_pair(0, CborValue());
}

static std::pair<size_t, CborValue> internalRead(const unsigned char *data, size_t size)
{
    unsigned char majorType = (data[0] & 0xe0) >> 5;
    unsigned char minorType = (data[0] & 0x1f);

    switch(majorType)
    {
        case UnsignedInt:
            // Unsigned integer
            return readPositiveInteger(minorType, data, size + 1);
            break;
        case NegativeInt:
            // Negative integer
            return readNegativeInteger(minorType, data, size + 1);
            break;
        case Bytes:
            // Byte string
            return readByteString(minorType, data, size + 1);
            break;
        case Utf8String:
            // Utf-8 string
            return readString(minorType, data, size + 1);
            break;
        case Array:
            // Array
            return readArray(minorType, data, size + 1);
            break;
        case Map:
            // Map
            return readMap(minorType, data, size + 1);
            break;
        case Tag:
            // Tagged
            return readTagger(minorType, data, size + 1);
            break;
        case Prim:
            // Simple or float
            return simpleOrFloat(minorType, data, size + 1);
            break;
    }

    std::cerr << "Internal error: invalid type " << static_cast<int>(majorType) << " "
              << static_cast<int>(minorType) << std::endl;
    assert(false);

    return std::make_pair(0, CborValue());
}

CborValue cborRead(const std::vector<char> &data)
{
    if( data.empty() )
        return CborValue();

    return internalRead(reinterpret_cast<const unsigned char *>(data.data()), data.size()).second;
}

