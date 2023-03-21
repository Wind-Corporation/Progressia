#include "image.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
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

    auto resource = __embedded_resources::getEmbeddedResource(path.c_str());

    if (resource.data == nullptr) {
        // REPORT_ERROR
        progressia::main::logging::fatal()
            << "Could not find resource \"" << path << "\"";
        exit(1);
    }

    std::vector<Image::Byte> png(resource.data,
                                 resource.data + resource.length);

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

} // namespace progressia::main
