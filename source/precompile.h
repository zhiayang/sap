// precompile.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <concepts>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <zpr.h>
#include <zst/zst.h>
#include <zst/SharedPtr.h>

// make ssize_t on windows
#ifdef _WIN32
using ssize_t = std::make_signed_t<size_t>;
#endif

#include "defs.h"
#include "pool.h"
#include "util.h"
#include "units.h"
#include "types.h"

namespace stdfs = std::filesystem;
