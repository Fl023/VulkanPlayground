#pragma once
#include "core/UUID.hpp"

// A simple 64-bit integer handle
using AssetHandle = UUID;

// We define '0' as our invalid/null handle
const AssetHandle INVALID_ASSET_HANDLE = { 0, 0 };