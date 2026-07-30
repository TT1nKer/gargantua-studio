#include "gargantua/reference/reference_output.h"

#include "reference_serialization.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

namespace gargantua::reference {
namespace {

ReferenceOutputResult failure(std::string message) {
    return ReferenceOutputResult{
        false, std::move(message), 0, 0};
}

bool write_bytes(
    const std::filesystem::path& path,
    const unsigned char* bytes,
    std::size_t size) {
    std::ofstream output(
        path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output.write(
        reinterpret_cast<const char*>(bytes),
        static_cast<std::streamsize>(size));
    output.close();
    return bool(output);
}

bool write_text(
    const std::filesystem::path& path,
    const std::string& text) {
    return write_bytes(
        path,
        reinterpret_cast<const unsigned char*>(text.data()),
        text.size());
}

} // namespace

ReferenceOutputResult write_reference_generation(
    const std::filesystem::path& output_directory,
    const ReferenceFrame& frame) {
    namespace fs = std::filesystem;
    if (output_directory.empty()) {
        return failure("output directory must not be empty");
    }

    const detail::SerializedReferenceGeneration serialized =
        detail::serialize_reference_generation(frame);
    if (!serialized.valid) {
        return failure(serialized.message);
    }

    const fs::path parent =
        output_directory.parent_path().empty()
            ? fs::path(".")
            : output_directory.parent_path();
    std::error_code error;
    if (!fs::is_directory(parent, error) || error) {
        return failure("output parent directory does not exist");
    }
    if (fs::exists(output_directory, error) || error) {
        return failure("output generation already exists");
    }

    fs::path part_directory = output_directory;
    part_directory += ".part";
    if (fs::exists(part_directory, error) || error) {
        return failure("partial output generation already exists");
    }
    if (!fs::create_directory(part_directory, error) || error) {
        return failure("cannot create partial output generation");
    }

    if (!write_bytes(
            part_directory / "classification.ppm",
            serialized.ppm.data(),
            serialized.ppm.size())) {
        return failure("cannot write classification.ppm");
    }
    if (!write_text(
            part_directory / "rays.csv",
            serialized.csv)) {
        return failure("cannot write rays.csv");
    }
    if (!write_text(
            part_directory / "manifest.json",
            serialized.manifest)) {
        return failure("cannot write manifest.json");
    }

    fs::rename(part_directory, output_directory, error);
    if (error) {
        return failure("cannot commit reference output generation");
    }
    return ReferenceOutputResult{
        true,
        {},
        serialized.ppm_checksum,
        serialized.csv_checksum,
    };
}

} // namespace gargantua::reference
