// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <type_traits>
#include <boost/range/iterator_range.hpp>
#include <glad/glad.h>
#include "common/common_types.h"
#include "common/math_util.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

enum class PixelFormat : u8 {
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

class FormatConverterBase {
public:
    virtual ~FormatConverterBase() = default;
    virtual void Convert(GLuint src_tex, const Common::Rectangle<u32>& src_rect,
                         GLuint read_fb_handle, GLuint dst_tex,
                         const Common::Rectangle<u32>& dst_rect, GLuint draw_fb_handle) = 0;
};

struct PixelFormatPair {
    const PixelFormat dst_format, src_format;
    struct Less {
        using is_transparent = void;
        constexpr bool operator()(OpenGL::PixelFormatPair lhs, OpenGL::PixelFormatPair rhs) const {
            return std::tie(lhs.dst_format, lhs.src_format) <
                   std::tie(rhs.dst_format, rhs.src_format);
        }
        constexpr bool operator()(OpenGL::PixelFormat lhs, OpenGL::PixelFormatPair rhs) const {
            return lhs < rhs.dst_format;
        }
        constexpr bool operator()(OpenGL::PixelFormatPair lhs, OpenGL::PixelFormat rhs) const {
            return lhs.dst_format < rhs;
        }
    };
};

class FormatConverterOpenGL : NonCopyable {
    using ConverterMap = std::map<PixelFormatPair, std::unique_ptr<FormatConverterBase>, PixelFormatPair::Less>;

public:
    FormatConverterOpenGL();
    ~FormatConverterOpenGL();

    boost::iterator_range<ConverterMap::iterator> GetPossibleConversions(PixelFormat dst_format);

private:
    ConverterMap converters;
};

} // namespace OpenGL
