// precompile.h
// Copyright (c) 2021, zhiayang
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

#include "defs.h"
#include "pool.h"
#include "util.h"
#include "units.h"
#include "types.h"

namespace stdfs = std::filesystem;
