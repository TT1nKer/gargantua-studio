#pragma once

#include "gargantua/reference/reference_scene.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gargantua::cli {

struct ReferenceRenderOptions {
    reference::ReferenceScene scene;
    std::filesystem::path output_directory;
};

struct ReferenceRenderParse {
    std::optional<ReferenceRenderOptions> options;
    bool show_help = false;
    std::string message;

    explicit operator bool() const noexcept {
        return show_help || options.has_value();
    }
};

ReferenceRenderParse parse_reference_render_options(
    const std::vector<std::string>& arguments);
std::string reference_render_usage();

} // namespace gargantua::cli
