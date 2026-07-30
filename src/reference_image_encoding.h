#pragma once

#include "gargantua/reference/reference_frame.h"

#include <vector>

namespace gargantua::reference::detail {

struct EncodedReferenceImages {
    std::vector<unsigned char> beauty_ppm;
    std::vector<unsigned char> classification_ppm;
};

EncodedReferenceImages encode_reference_images(
    const ReferenceFrame& frame);

} // namespace gargantua::reference::detail
