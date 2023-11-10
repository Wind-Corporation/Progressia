#pragma once

#include <filesystem>
#include <vector>

namespace progressia::main {

class Image {
  public:
    using Byte = unsigned char;

    std::size_t width;
    std::size_t height;
    std::vector<Byte> data;

    std::size_t getSize() const;
    const Byte *getData() const;
    Byte *getData();
};

Image loadImage(const std::string &);

} // namespace progressia::main
