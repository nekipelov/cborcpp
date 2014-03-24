/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef CBORREADER_H
#define CBORREADER_H

#include <vector>

#include "value.h"

Value cborRead(const std::vector<char> &data);

#endif // CBORREADER_H
