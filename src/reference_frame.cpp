#include "gargantua/reference/reference_frame.h"

namespace gargantua::reference {

const char* frame_status_name(FrameStatus status) noexcept {
    switch (status) {
    case FrameStatus::Complete:
        return "complete";
    case FrameStatus::DiagnosticFailed:
        return "diagnostic_failed";
    }
    return "unknown";
}

} // namespace gargantua::reference
