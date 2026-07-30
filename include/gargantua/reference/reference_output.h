#pragma once

#include "gargantua/reference/reference_frame.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace gargantua::reference {

struct ReferenceOutputResult {
    bool written;
    std::string message;
    std::uint64_t beauty_ppm_checksum;
    std::uint64_t classification_ppm_checksum;
    std::uint64_t csv_checksum;

    explicit operator bool() const noexcept {
        return written;
    }
};

ReferenceOutputResult write_reference_generation(
    const std::filesystem::path& output_directory,
    const ReferenceFrame& frame);

} // namespace gargantua::reference
