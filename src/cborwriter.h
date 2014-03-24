/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef CBORWRITER_H
#define CBORWRITER_H

#include <vector>

#include "value.h"

std::vector<char> cborWrite(const Value &value);

#endif // CBORWRITER_H
