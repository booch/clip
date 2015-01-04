/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_STDTYPES_H
#define _FNORD_STDTYPES_H
#include <ctime>
#include <deque>
#include <inttypes.h>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace fnord {

using String = std::string;

using Unit = decltype(nullptr);

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using List = std::list<T>;

template <typename T>
using Deque = std::deque<T>;

template <typename T>
using UniqueRef = std::unique_ptr<T>;

template <typename T>
using Function = std::function<T>;

} // namespace fnord
#endif
