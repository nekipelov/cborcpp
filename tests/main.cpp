#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <stdio.h>
#include <boost/test/unit_test.hpp>
#include <math.h>

#include "../src/cborcpp.h"
#include "../src/cborvalue.h"


template<size_t N>
std::vector<char> toVector(const char (&ptr)[N])
{
    if( N > 1 )
        return std::vector<char>(ptr, ptr + N - 1);
    else
        return std::vector<char>();
}

CborValue decode(const std::vector<char> &data)
{
    return cborRead(data);
}

template<typename T>
std::vector<char> encode(const T &value)
{
    return cborWrite(value);
}

std::ostream & operator << (std::ostream &stream, const CborValue &v)
{
    stream << v.inspect();
    return stream;
}

BOOST_AUTO_TEST_CASE( PositiveNumbers )
{
    BOOST_CHECK_EQUAL(0, decode(toVector("\x00")));
    BOOST_CHECK_EQUAL(1, decode(toVector("\x01")));
    BOOST_CHECK_EQUAL(10, decode(toVector("\x0A")));
    BOOST_CHECK_EQUAL(24, decode(toVector("\x18\x18")));
    BOOST_CHECK_EQUAL(25, decode(toVector("\x18\x19")));
    BOOST_CHECK_EQUAL(255, decode(toVector("\x18\xFF")));
    BOOST_CHECK_EQUAL(256, decode(toVector("\x19\x01\x00")));
    BOOST_CHECK_EQUAL(65535, decode(toVector("\x19\xFF\xFF")));
    BOOST_CHECK_EQUAL(65536, decode(toVector("\x1A\x00\x01\x00\x00")));
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(4294967295ULL), decode(toVector("\x1A\xFF\xFF\xFF\xFF")));
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(4294967296ULL), decode(toVector("\x1B\x00\x00\x00\x01\x00\x00\x00\x00")));
    BOOST_CHECK_EQUAL(1000000, decode(toVector("\x1a\x00\x0f\x42\x40")));
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(1000000000000ULL), decode(toVector("\x1b\x00\x00\x00\xe8\xd4\xa5\x10\x00")));
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(18446744073709551615ULL), decode(toVector("\x1b\xff\xff\xff\xff\xff\xff\xff\xff")));

    BOOST_VERIFY(encode(0) == toVector("\x00"));
    BOOST_VERIFY(encode(1) == toVector("\x01"));
    BOOST_VERIFY(encode(10) == toVector("\x0A"));
    BOOST_VERIFY(encode(24) == toVector("\x18\x18"));
    BOOST_VERIFY(encode(25) == toVector("\x18\x19"));
    BOOST_VERIFY(encode(255) == toVector("\x18\xFF"));
    BOOST_VERIFY(encode(256) == toVector("\x19\x01\x00"));
    BOOST_VERIFY(encode(65535) == toVector("\x19\xFF\xFF"));
    BOOST_VERIFY(encode(65536) == toVector("\x1A\x00\x01\x00\x00"));
    BOOST_VERIFY(encode(static_cast<uint64_t>(4294967295ULL)) == toVector("\x1A\xFF\xFF\xFF\xFF"));
    BOOST_VERIFY(encode(static_cast<uint64_t>(4294967296ULL)) == toVector("\x1B\x00\x00\x00\x01\x00\x00\x00\x00"));
    BOOST_VERIFY(encode(1000000) == toVector("\x1a\x00\x0f\x42\x40"));
    BOOST_VERIFY(encode(static_cast<uint64_t>(1000000000000ULL)) == toVector("\x1b\x00\x00\x00\xe8\xd4\xa5\x10\x00"));
    BOOST_VERIFY(encode(static_cast<uint64_t>(18446744073709551615ULL)) ==  toVector("\x1b\xff\xff\xff\xff\xff\xff\xff\xff"));
}

BOOST_AUTO_TEST_CASE( NegativeNumbers )
{
    BOOST_CHECK_EQUAL(-16, decode(toVector("\x2F")));
    BOOST_CHECK_EQUAL(-1, decode(toVector("\x20")));
    BOOST_CHECK_EQUAL(-10, decode(toVector("\x29")));
    BOOST_CHECK_EQUAL(-24, decode(toVector("\x37")));
    BOOST_CHECK_EQUAL(-25, decode(toVector("\x38\x18")));
    BOOST_CHECK_EQUAL(-250, decode(toVector("\x38\xF9")));
    BOOST_CHECK_EQUAL(-256, decode(toVector("\x38\xFF")));
    BOOST_CHECK_EQUAL(-65535, decode(toVector("\x39\xFF\xFE")));
    BOOST_CHECK_EQUAL(-65536, decode(toVector("\x39\xFF\xFF")));
    BOOST_CHECK_EQUAL(static_cast<int64_t>(-4294967295LL), decode(toVector("\x3A\xFF\xFF\xFF\xFE")));
    BOOST_CHECK_EQUAL(static_cast<int64_t>(-4294967296LL), decode(toVector("\x3A\xFF\xFF\xFF\xFF")));

    {
        // -18446744073709551616LL too big for 64-bit integer. This value must
        // be writed as BigInteger. But encoded as 64-bit integer.
        CborValue::BigInteger bitInteger;

        bitInteger.positive = false;
        const char value[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        bitInteger.bigint.assign(value, value + sizeof(value));

        BOOST_CHECK_EQUAL(CborValue(bitInteger), decode(toVector("\x3b\xff\xff\xff\xff\xff\xff\xff\xff")));
        BOOST_VERIFY(encode(bitInteger) == toVector("\x3b\xff\xff\xff\xff\xff\xff\xff\xff"));
    }

    BOOST_VERIFY(encode(-16) == toVector("\x2F"));
    BOOST_VERIFY(encode(-1) == toVector("\x20"));
    BOOST_VERIFY(encode(-10) == toVector("\x29"));
    BOOST_VERIFY(encode(-24) == toVector("\x37"));
    BOOST_VERIFY(encode(-25) == toVector("\x38\x18"));
    BOOST_VERIFY(encode(-250) == toVector("\x38\xF9"));
    BOOST_VERIFY(encode(-256) == toVector("\x38\xFF"));
    BOOST_VERIFY(encode(-65535) == toVector("\x39\xFF\xFE"));
    BOOST_VERIFY(encode(-65536) == toVector("\x39\xFF\xFF"));
    BOOST_VERIFY(encode(static_cast<int64_t>(-4294967295LL)) == toVector("\x3A\xFF\xFF\xFF\xFE"));
    BOOST_VERIFY(encode(static_cast<int64_t>(-4294967296LL)) == toVector("\x3A\xFF\xFF\xFF\xFF"));
}

BOOST_AUTO_TEST_CASE( FloatNumbers )
{
    // Decoder
    BOOST_CHECK_EQUAL(0.0, decode(toVector("\xf9\x00\x00")));
    BOOST_CHECK_EQUAL(-0.0, decode(toVector("\xf9\x80\x00")));
    BOOST_CHECK_EQUAL(1.0, decode(toVector("\xf9\x3c\x00")));
    BOOST_CHECK_EQUAL(1.1, decode(toVector("\xfb\x3f\xf1\x99\x99\x99\x99\x99\x9a")));
    BOOST_CHECK_EQUAL(1.5, decode(toVector("\xf9\x3e\x00")));
    BOOST_CHECK_EQUAL(65504.0, decode(toVector("\xf9\x7b\xff")));
    BOOST_CHECK_EQUAL(100000.0, decode(toVector("\xfa\x47\xc3\x50\x00")));
    BOOST_CHECK_EQUAL(3.4028234663852886e+38, decode(toVector("\xfa\x7f\x7f\xff\xff")));
    BOOST_CHECK_EQUAL(1.0e+300, decode(toVector("\xfb\x7e\x37\xe4\x3c\x88\x00\x75\x9c")));
    BOOST_CHECK_EQUAL(5.960464477539063e-8, decode(toVector("\xf9\x00\x01")));
    BOOST_CHECK_EQUAL(0.00006103515625, decode(toVector("\xf9\x04\x00")));
    BOOST_CHECK_EQUAL(-4.0, decode(toVector("\xf9\xc4\x00")));
    BOOST_CHECK_EQUAL(-4.1, decode(toVector("\xfb\xc0\x10\x66\x66\x66\x66\x66\x66")));
    BOOST_CHECK( isinf(decode(toVector("\xf9\x7c\x00")).toDouble()) );
    BOOST_CHECK( isnan(decode(toVector("\xf9\x7e\x00")).toDouble()) );
    BOOST_CHECK( isinf(decode(toVector("\xf9\xfc\x00")).toDouble()) );

    BOOST_CHECK( isinf(decode(toVector("\xfa\x7f\x80\x00\x00")).toDouble()) );
    BOOST_CHECK( isnan(decode(toVector("\xfa\x7f\xc0\x00\x00")).toDouble()) );
    BOOST_CHECK( isinf(decode(toVector("\xfa\xff\x80\x00\x00")).toDouble()) );

    BOOST_CHECK( isinf(decode(toVector("\xfb\x7f\xf0\x00\x00\x00\x00\x00\x00")).toDouble()) );
    BOOST_CHECK( isnan(decode(toVector("\xfb\x7f\xf8\x00\x00\x00\x00\x00\x00")).toDouble()) );
    BOOST_CHECK( isinf(decode(toVector("\xfb\xff\xf0\x00\x00\x00\x00\x00\x00")).toDouble()) );

    // Encoder
    BOOST_CHECK(encode(0.0) == toVector("\xf9\x00\x00"));
    BOOST_CHECK(encode(-0.0) == toVector("\xf9\x80\x00"));
    BOOST_CHECK(encode(1.0) == toVector("\xf9\x3c\x00"));
    BOOST_CHECK(encode(1.1) == toVector("\xfb\x3f\xf1\x99\x99\x99\x99\x99\x9a"));
    BOOST_CHECK(encode(1.5) == toVector("\xf9\x3e\x00"));
    BOOST_CHECK(encode(65504.0) == toVector("\xf9\x7b\xff"));
    BOOST_CHECK(encode(100000.0) == toVector("\xfa\x47\xc3\x50\x00"));
    BOOST_CHECK(encode(3.4028234663852886e+38) == toVector("\xfa\x7f\x7f\xff\xff"));
    BOOST_CHECK(encode(1.0e+300) == toVector("\xfb\x7e\x37\xe4\x3c\x88\x00\x75\x9c"));
    BOOST_CHECK(encode(5.960464477539063e-8) == toVector("\xf9\x00\x01"));
    BOOST_CHECK(encode(0.00006103515625) == toVector("\xf9\x04\x00"));
    BOOST_CHECK(encode(-4.0) == toVector("\xf9\xc4\x00"));
    BOOST_CHECK(encode(-4.1) == toVector("\xfb\xc0\x10\x66\x66\x66\x66\x66\x66"));
    BOOST_CHECK(encode(std::numeric_limits<float>::infinity()) == toVector("\xf9\x7c\x00"));
    BOOST_CHECK(encode(std::numeric_limits<float>::quiet_NaN()) == toVector("\xf9\x7e\x00"));
    BOOST_CHECK(encode(-std::numeric_limits<float>::infinity()) == toVector("\xf9\xfc\x00"));
    BOOST_CHECK(encode(std::numeric_limits<double>::infinity()) == toVector("\xf9\x7c\x00"));
    BOOST_CHECK(encode(std::numeric_limits<double>::quiet_NaN()) == toVector("\xf9\x7e\x00"));
    BOOST_CHECK(encode(-std::numeric_limits<double>::infinity()) == toVector("\xf9\xfc\x00"));
}

BOOST_AUTO_TEST_CASE( BigNumbers )
{
    {
        // 18446744073709551617
        const char bigNumData [] = "\x01\x00\x00\x00\x00\x00\x00\x00\x01";
        const std::vector<char> bigNumBuf(bigNumData, bigNumData + sizeof(bigNumData) - 1);
        const char encoded[] = "\xc2\x49\x01\x00\x00\x00\x00\x00\x00\x00\x01";
        CborValue::BigInteger bigInteger;

        bigInteger.positive = true;
        bigInteger.bigint = bigNumBuf;

        BOOST_CHECK_EQUAL(CborValue(bigInteger), decode(toVector(encoded)));
        BOOST_CHECK(encode(CborValue(bigInteger)) == toVector(encoded));
    }

    {
        // -18446744073709551617
        const char bigNumData [] = "\x01\x00\x00\x00\x00\x00\x00\x00\x01";
        const std::vector<char> bigNumBuf(bigNumData, bigNumData + sizeof(bigNumData) - 1);
        const char encoded[] = "\xc3\x49\x01\x00\x00\x00\x00\x00\x00\x00\x00";
        CborValue::BigInteger bigInteger;

        bigInteger.positive = false;
        bigInteger.bigint = bigNumBuf;

        BOOST_CHECK_EQUAL(CborValue(bigInteger), decode(toVector(encoded)));
        BOOST_CHECK(encode(CborValue(bigInteger)) == toVector(encoded));
    }
}

BOOST_AUTO_TEST_CASE( SimpleValues )
{
    BOOST_CHECK_EQUAL(false, decode(toVector("\xf4")).toBool());
    BOOST_CHECK_EQUAL(true, decode(toVector("\xf5")).toBool());
    BOOST_CHECK(decode(toVector("\xf6")).type() == CborValue::NullType);
    BOOST_CHECK(decode(toVector("\xf7")).type() == CborValue::UndefinedType);
    BOOST_CHECK(encode(CborValue(false)) == toVector("\xf4"));
    BOOST_CHECK(encode(CborValue(true)) == toVector("\xf5"));
    BOOST_CHECK(encode(CborValue(CborValue::NullTag())) == toVector("\xf6"));
    BOOST_CHECK(encode(CborValue(CborValue::UndefinedTag())) == toVector("\xf7"));


//        | simple(16)                   | 0xf0                               |
//        |                              |                                    |
//        | simple(24)                   | 0xf818                             |
//        |                              |                                    |
//        | simple(255)                  | 0xf8ff                             |
//        |                              |                                    |
}

BOOST_AUTO_TEST_CASE( TaggedValues )
{
//        | 0("2013-03-21T20:04:00Z")    | 0xc074323031332d30332d32315432303a |
//        |                              | 30343a30305a                       |
//        |                              |                                    |
//        | 1(1363896240)                | 0xc11a514b67b0                     |
//        |                              |                                    |
//        | 1(1363896240.5)              | 0xc1fb41d452d9ec200000             |
//        |                              |                                    |
//        | 23(h'01020304')              | 0xd74401020304                     |
//        |                              |                                    |
//        | 24(h'6449455446')            | 0xd818456449455446                 |
//        |                              |                                    |
//        | 32("http://www.example.com") | 0xd82076687474703a2f2f7777772e6578 |
//        |                              | 616d706c652e636f6d                 |
//        |                              |                                    |
//        | h''                          | 0x40                               |
//        |                              |                                    |
//        | h'01020304'                  | 0x4401020304                       |
}

BOOST_AUTO_TEST_CASE( StringValues )
{
    // strings
    BOOST_CHECK_EQUAL("hello world!", decode(toVector("\x6C\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64\x21")));
    BOOST_CHECK_EQUAL("", decode(toVector("\x60")));
    BOOST_CHECK_EQUAL("a", decode(toVector("\x61\x61")));
    BOOST_CHECK_EQUAL("IETF", decode(toVector("\x64\x49\x45\x54\x46")));
    BOOST_CHECK_EQUAL("\"\\", decode(toVector("\x62\x22\x5c")));

    BOOST_CHECK(encode("hello world!") == toVector("\x6C\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64\x21"));
    BOOST_CHECK(encode("") == toVector("\x60"));
    BOOST_CHECK(encode("a") == toVector("\x61\x61"));
    BOOST_CHECK(encode("IETF") == toVector("\x64\x49\x45\x54\x46"));
    BOOST_CHECK(encode("\"\\") == toVector("\x62\x22\x5c"));

    BOOST_CHECK(encode("\u00fc") == toVector("\x62\xc3\xbc"));
    BOOST_CHECK(encode("\u6c34") == toVector("\x63\xe6\xb0\xb4"));

    //        | "\ud800\udd51"               | 0x64f0908591                       |
}

BOOST_AUTO_TEST_CASE( MapsAndArrayValues )
{
    BOOST_CHECK_EQUAL(std::vector<CborValue>(), decode(toVector("\x80")));
    BOOST_CHECK(encode(std::vector<CborValue>()) == toVector("\x80"));

    {
        std::vector<CborValue>  arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        BOOST_CHECK_EQUAL(arr, decode(toVector("\x83\x01\x02\x03")));
        BOOST_CHECK(encode(arr) == toVector("\x83\x01\x02\x03"));
    }

    {
        std::vector<CborValue> arr1;
        std::vector<CborValue> arr2;
        std::vector<CborValue> arr;

        arr1.push_back(2);
        arr1.push_back(3);

        arr2.push_back(4);
        arr2.push_back(5);

        arr.push_back(1);
        arr.push_back(arr1);
        arr.push_back(arr2);

        BOOST_CHECK_EQUAL(arr, decode(toVector("\x83\x01\x82\x02\x03\x82\x04\x05")));
        BOOST_CHECK(encode(arr) == toVector("\x83\x01\x82\x02\x03\x82\x04\x05"));
    }

    {
        std::vector<CborValue> arr;

        for(int i = 1; i <= 25; ++i)
            arr.push_back(i);

        std::vector<char> data =  toVector("\x98\x19\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c"
                                           "\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x18\x18\x19");

        BOOST_CHECK_EQUAL(arr, decode(data));
        BOOST_CHECK(encode(arr) == data);
    }


    {
        std::map<CborValue, CborValue> emptyMap;

        BOOST_CHECK_EQUAL(emptyMap, decode(toVector("\xa0")));        
        BOOST_CHECK(encode(emptyMap) == toVector("\xa0"));
    }

    {
        std::map<CborValue, CborValue> map;

        map[CborValue(1)] = CborValue(2);
        map[CborValue(3)] = CborValue(4);

        BOOST_CHECK_EQUAL(map, decode(toVector("\xa2\x01\x02\x03\x04")));        
        BOOST_CHECK(encode(map) == toVector("\xa2\x01\x02\x03\x04"));
    }

    {
        std::vector<CborValue> arr;
        std::map<CborValue, CborValue> map;

        arr.push_back(2);
        arr.push_back(3);

        map.insert(std::make_pair(CborValue("a"), CborValue(1)));
        map.insert(std::make_pair(CborValue("b"), arr));

        BOOST_CHECK_EQUAL(map, decode(toVector("\xa2\x61\x61\x01\x61\x62\x82\x02\x03")));
        BOOST_CHECK(encode(map) == toVector("\xa2\x61\x61\x01\x61\x62\x82\x02\x03"));
    }

    {
        std::map<CborValue, CborValue> map;
        std::vector<CborValue> array;

        map[CborValue("b")] = CborValue("c");
        array.push_back("a");
        array.push_back(map);

        BOOST_CHECK_EQUAL(array, decode(toVector("\x82\x61\x61\xa1\x61\x62\x61\x63")));
        BOOST_CHECK(encode(array) == toVector("\x82\x61\x61\xa1\x61\x62\x61\x63"));
    }

    {
        std::map<CborValue, CborValue> map;

        map[CborValue("a")] = CborValue("A");
        map[CborValue("b")] = CborValue("B");
        map[CborValue("c")] = CborValue("C");
        map[CborValue("d")] = CborValue("D");
        map[CborValue("e")] =  CborValue("E");

        std::vector<char> data = toVector("\xa5\x61\x61\x61\x41\x61\x62\x61\x42\x61\x63"
                                          "\x61\x43\x61\x64\x61\x44\x61\x65\x61\x45");


        BOOST_CHECK_EQUAL(map, decode(data));
        BOOST_CHECK(encode(map) == data);
    }

    //        | [_ ]                         | 0x9fff                             |
    //        |                              |                                    |
    //        | [_ 1, [2, 3], [_ 4, 5]]      | 0x9f018202039f0405ffff             |
    //        |                              |                                    |
    //        | [_ 1, [2, 3], [4, 5]]        | 0x9f01820203820405ff               |
    //        |                              |                                    |
    //        | [1, [2, 3], [_ 4, 5]]        | 0x83018202039f0405ff               |
    //        |                              |                                    |
    //        | [1, [_ 2, 3], [4, 5]]        | 0x83019f0203ff820405               |
    //        |                              |                                    |
    //        | [_ 1, 2, 3, 4, 5, 6, 7, 8,   | 0x9f0102030405060708090a0b0c0d0e0f |
    //        | 9, 10, 11, 12, 13, 14, 15,   | 101112131415161718181819ff         |
    //        | 16, 17, 18, 19, 20, 21, 22,  |                                    |
    //        | 23, 24, 25]                  |                                    |
    //        |                              |                                    |
    //        | {_ "a": 1, "b": [_ 2, 3]}    | 0xbf61610161629f0203ffff           |
    //        |                              |                                    |
}


BOOST_AUTO_TEST_CASE( BinaryString )
{
    {
        const char binaryData[] = "\0binary string\0";
        std::vector<char> binaryBuf(binaryData, binaryData + sizeof(binaryData) - 1);

        const char encoded[] = "\x4F\x00\x62\x69\x6E\x61\x72\x79"
                               "\x20\x73\x74\x72\x69\x6E\x67\x0";

        BOOST_CHECK(encode(binaryBuf) == toVector(encoded));
        BOOST_CHECK(binaryBuf == decode(toVector(encoded)));
    }
//        |                              |                                    |
//        | (_ h'0102', h'030405')       | 0x5f42010243030405ff               |
//        |                              |                                    |
//        | (_ "strea", "ming")          | 0x7f657374726561646d696e67ff       |
//        |                              |                                    |
}

BOOST_AUTO_TEST_CASE( Interface )
{
    {
        std::map<CborValue, CborValue> map;

        map[CborValue("a")] = CborValue("A");
        map[CborValue("b")] = CborValue("B");
        map[CborValue("c")] = CborValue("C");

        CborValue value(map);

        BOOST_CHECK(value.size() == 3);
        BOOST_CHECK(value.hasMember("a"));
        BOOST_CHECK(value.hasMember("b"));
        BOOST_CHECK(value.hasMember("c"));
        BOOST_CHECK(value.hasMember("A") == false );
        BOOST_CHECK(value.hasMember("B") == false );
        BOOST_CHECK(value.hasMember("C") == false );

        BOOST_CHECK(value.member("a") == CborValue("A"));
        BOOST_CHECK(value.member("b") == CborValue("B"));
        BOOST_CHECK(value.member("c") == CborValue("C"));
    }

    {
        std::vector<CborValue> arr;

        arr.push_back(CborValue("a"));
        arr.push_back(CborValue("b"));
        arr.push_back(CborValue(1));
        arr.push_back(CborValue(2));

        CborValue value(arr);

        BOOST_CHECK(value.size() == 4);

        BOOST_CHECK(value.at(0) == CborValue("a"));
        BOOST_CHECK(value.at(1) == CborValue("b"));
        BOOST_CHECK(value.at(2) == CborValue(1));
        BOOST_CHECK(value.at(3) == CborValue(2));
    }

    {
        std::map<CborValue, CborValue> map;

        map[CborValue("a")] = CborValue("A");
        map[CborValue("b")] = CborValue("B");


        CborValue cborMap(map);
        CborValue::Iterator it(cborMap);

        BOOST_CHECK(it.hasNext() == true);
        BOOST_CHECK(it.hasPrev() == false);
        BOOST_CHECK(it.next() == CborValue("A"));
        BOOST_CHECK(it.key() == CborValue("a"));
        BOOST_CHECK(it.value() == CborValue("A"));

        BOOST_CHECK(it.next() == CborValue("B"));
        BOOST_CHECK(it.hasNext() == false);
        BOOST_CHECK(it.hasPrev() == true);
        BOOST_CHECK(it.key() == CborValue("b"));
        BOOST_CHECK(it.value() == CborValue("B"));
    }

    {
        std::vector<CborValue> arr;

        arr.push_back(CborValue("A"));
        arr.push_back(CborValue("B"));


        CborValue cborArr(arr);
        CborValue::Iterator it(cborArr);

        BOOST_CHECK(it.hasNext() == true);
        BOOST_CHECK(it.hasPrev() == false);
        BOOST_CHECK(it.next() == CborValue("A"));
        BOOST_CHECK(it.key() == CborValue(0));
        BOOST_CHECK(it.value() == CborValue("A"));

        BOOST_CHECK(it.next() == CborValue("B"));

        BOOST_CHECK(it.hasNext() == false);
        BOOST_CHECK(it.hasPrev() == true);
        BOOST_CHECK(it.key() == CborValue(1));
        BOOST_CHECK(it.value() == CborValue("B"));
    }
}
