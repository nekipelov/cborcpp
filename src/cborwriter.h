/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef CBORWRITER_H
#define CBORWRITER_H

#include <vector>

#include "cborvalue.h"

std::vector<char> cborWrite(const CborValue &value);

#endif // CBORWRITER_H
