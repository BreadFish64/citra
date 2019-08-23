// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once
#include <glad/glad.h>
#include "common/common_types.h"
#include "common/math_util.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

enum class PixelFormat {
    // First 5 formats are shared between textures and color buffers
    RGBA8 = 0,
    RGB8 = 1,
    RGB5A1 = 2,
    RGB565 = 3,
    RGBA4 = 4,

    // Texture-only formats
    IA8 = 5,
    RG8 = 6,
    I8 = 7,
    A8 = 8,
    IA4 = 9,
    I4 = 10,
    A4 = 11,
    ETC1 = 12,
    ETC1A4 = 13,

    // Depth buffer-only formats
    D16 = 14,
    // gap
    D24 = 16,
    D24S8 = 17,

    Invalid = 255,
};

class FormatConverterBase;

class FormatConverterOpenGL : NonCopyable {
public:
    FormatConverterOpenGL();
    ~FormatConverterOpenGL();

    std::vector<PixelFormat> GetPossibleConversions(PixelFormat dst_format) const;

    bool Convert(PixelFormat src_format, GLuint src_tex, const Common::Rectangle<u32>& src_rect,
                 GLuint read_fb_handle, PixelFormat dst_format, GLuint dst_tex,
                 const Common::Rectangle<u32>& dst_rect, GLuint draw_fb_handle);

private:
    using FromToFormatPair = std::pair<PixelFormat, PixelFormat>;
    std::map<FromToFormatPair, std::shared_ptr<FormatConverterBase>> converters;
};

} // namespace OpenGL
