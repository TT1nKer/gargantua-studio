#pragma once

#include "gargantua/reference/reference_frame.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gargantua::reference::detail {

struct SerializedReferenceGeneration {
    bool valid = false;
    std::string message;
    std::vector<unsigned char> ppm;
    std::string csv;
    std::string manifest;
    std::uint64_t ppm_checksum = 0;
    std::uint64_t csv_checksum = 0;
};

SerializedReferenceGeneration serialize_reference_generation(
    const ReferenceFrame& frame);

} // namespace gargantua::reference::detail
