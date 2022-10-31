#include "image.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "stb/stb_image.h"

#include "../logging.h"
using namespace progressia::main::logging;

namespace progressia {
namespace main {

std::size_t Image::getSize() const { return data.size(); }

const Image::Byte *Image::getData() const { return data.data(); }

Image::Byte *Image::getData() { return data.data(); }

Image loadImage(const std::filesystem::path &path) {

    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        fatal() << "Could not access a PNG image in file " << path;
        // REPORT_ERROR
        exit(1);
    }

    std::size_t fileSize = static_cast<std::size_t>(file.tellg());
    std::vector<Image::Byte> png(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char *>(png.data()), fileSize);

    file.close();

    int width;
    int height;
    int channelsInFile;

    Image::Byte *stbAllocatedData =
        stbi_load_from_memory(png.data(), png.size(), &width, &height,
                              &channelsInFile, STBI_rgb_alpha);

    if (stbAllocatedData == NULL) {
        fatal() << "Could not decode a PNG image from file " << path;
        // REPORT_ERROR
        exit(1);
    }

    std::vector<Image::Byte> data(width * height * STBI_rgb_alpha);
    memcpy(data.data(), stbAllocatedData, data.size());

    stbi_image_free(stbAllocatedData);

    return {static_cast<std::size_t>(width), static_cast<std::size_t>(height),
            data};
}

} // namespace main
} // namespace progressia
