#pragma once

#include "gargantua/reference/reference_scene.h"
#include "solar/relativity/thin_disk.h"

namespace gargantua::reference::detail {

void validate_solar_thin_disk_scene(
    const ReferenceScene& scene);

solar::relativity::ThinDiskCrossingRecorder
make_solar_thin_disk_recorder(
    const ReferenceScene& scene);

} // namespace gargantua::reference::detail
