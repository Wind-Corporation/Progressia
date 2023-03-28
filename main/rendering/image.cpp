#include "image.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

#include "stb/stb_image.h"

#include <embedded_resources.h>

#include "../logging.h"
using namespace progressia::main::logging;

namespace progressia::main {

std::size_t Image::getSize() const { return data.size(); }

const Image::Byte *Image::getData() const { return data.data(); }

Image::Byte *Image::getData() { return data.data(); }

Image loadImage(const std::string &path) {

    auto resource = __embedded_resources::getEmbeddedResource(path);

    if (resource.data == nullptr) {
        // REPORT_ERROR
        progressia::main::logging::fatal()
            << "Could not find resource \"" << path << "\"";
        exit(1);
    }

    std::vector<Image::Byte> png(resource.data,
                                 resource.data + resource.length);

    if (png.size() > std::numeric_limits<int>::max()) {
        // REPORT_ERROR
        progressia::main::logging::fatal()
            << "Could not load \"" << path << "\": image file too large";
        exit(1);
    }

    int dataSize = static_cast<int>(png.size());
    int width = 0;
    int height = 0;
    int channelsInFile = 0;

    Image::Byte *stbAllocatedData = stbi_load_from_memory(
        png.data(), dataSize, &width, &height, &channelsInFile, STBI_rgb_alpha);

    if (stbAllocatedData == nullptr) {
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

} // namespace progressia::main
