/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef CBORPRIVATE_H
#define CBORPRIVATE_H

enum Types {
    UnsignedInt = 0,
    NegativeInt = 1,
    Bytes = 2,
    Utf8String = 3,
    Array = 4,
    Map = 5,
    Tag = 6,
    Prim = 7,
};

enum {
    FalseValue = 0x14,
    TrueValue = 0x15,
    NullValue = 0x16,
    UndefiendValue = 0x17,
    SimpleValue1Byte = 0x18,
    HalfPrecisionFloat = 0x19,
    SinglePrecisionFloat = 0x1a,
    DoublePrecisionFloat = 0x1b
};

#endif // CBORPRIVATE_H
