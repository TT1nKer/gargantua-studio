#include "reference_render_options.h"

#include "gargantua/reference/reference_output.h"
#include "gargantua/reference/reference_renderer.h"
#include "gargantua/reference/reference_ray_tracer.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    using namespace gargantua;

    try {
        std::vector<std::string> arguments;
        arguments.reserve(static_cast<std::size_t>(argc));
        for (int index = 0; index < argc; ++index) {
            arguments.emplace_back(argv[index]);
        }

        const cli::ReferenceRenderParse parsed =
            cli::parse_reference_render_options(arguments);
        if (!parsed) {
            std::cerr << parsed.message << '\n'
                      << cli::reference_render_usage();
            return 2;
        }
        if (parsed.show_help) {
            std::cout << cli::reference_render_usage();
            return 0;
        }

        reference::ReferenceTracerBuild tracer =
            reference::make_solar_kerr_ray_tracer(
                parsed.options->scene);
        if (!tracer) {
            std::cerr << tracer.message << '\n';
            return 3;
        }

        const reference::ReferenceRenderResult rendered =
            reference::render_reference_frame(
                parsed.options->scene, *tracer.tracer);
        if (!rendered) {
            std::cerr << rendered.message << '\n';
            return 3;
        }

        const reference::ReferenceOutputResult output =
            reference::write_reference_generation(
                parsed.options->output_directory,
                *rendered.frame);
        if (!output) {
            std::cerr << output.message << '\n';
            return 5;
        }

        const reference::ReferenceFrameSummary& summary =
            rendered.frame->summary;
        std::cout
            << "{\"status\":\""
            << reference::frame_status_name(rendered.frame->status)
            << "\",\"captured\":" << summary.captured
            << ",\"escaped\":" << summary.escaped
            << ",\"disk_surface_hits\":"
            << summary.disk_surface_hits
            << ",\"disk_crossings\":"
            << summary.disk_crossings
            << ",\"failed\":" << summary.failed
            << ",\"beauty_ppm_checksum_fnv1a64\":\""
            << std::hex << std::setw(16) << std::setfill('0')
            << output.beauty_ppm_checksum
            << "\",\"classification_ppm_checksum_fnv1a64\":\""
            << std::hex << std::setw(16) << std::setfill('0')
            << output.classification_ppm_checksum
            << "\",\"csv_checksum_fnv1a64\":\""
            << std::setw(16) << output.csv_checksum
            << "\"}\n";
        return rendered.frame->status ==
                       reference::FrameStatus::Complete
                   ? 0
                   : 4;
    } catch (const std::exception& error) {
        std::cerr << "reference render failed: "
                  << error.what() << '\n';
        return 3;
    }
}
