#pragma once

#include "gargantua/reference/reference_frame.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace gargantua::reference::detail {

std::string serialize_reference_manifest(
    const ReferenceFrame& frame,
    std::uint64_t scene_hash,
    std::uint64_t beauty_ppm_checksum,
    std::size_t beauty_ppm_bytes,
    std::uint64_t classification_ppm_checksum,
    std::size_t classification_ppm_bytes,
    std::uint64_t csv_checksum,
    std::size_t csv_bytes);

} // namespace gargantua::reference::detail
