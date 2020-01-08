// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include <vector>
#include "common/assert.h"
#include "common/scope_exit.h"
#include "video_core/renderer_opengl/gl_format_converter.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/gl_vars.h"

namespace OpenGL {

constexpr std::size_t FormatToIndex(const PixelFormat& src, const PixelFormat& dst) {
    AvailableConverters f(AvailableConverters::Unavailable);
    if (src == PixelFormat::D24S8 && dst == PixelFormat::RGBA8) {
        f = AvailableConverters::ReadPixel_D24S8_ABGR8;
    }
    return static_cast<std::size_t>(f);
}

class FormatConverterBase {
public:
    virtual ~FormatConverterBase() = default;
    virtual void Convert(GLuint src_tex, const Common::Rectangle<u32>& src_rect,
                         GLuint read_fb_handle, GLuint dst_tex,
                         const Common::Rectangle<u32>& dst_rect, GLuint draw_fb_handle) = 0;
};

class ConverterD24S8toABGR final : public FormatConverterBase {
public:
    ConverterD24S8toABGR() {
        attributeless_vao.Create();
        d24s8_abgr_buffer.Create();
        d24s8_abgr_buffer_size = 0;

        std::string vs_source = R"(
const vec2 vertices[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0));
void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
)";

        std::string fs_source = GLES ? fragment_shader_precision_OES : "";
        fs_source += R"(
uniform samplerBuffer tbo;
uniform vec2 tbo_size;
uniform vec4 viewport;

out vec4 color;

void main() {
    vec2 tbo_coord = (gl_FragCoord.xy - viewport.xy) * tbo_size / viewport.zw;
    int tbo_offset = int(tbo_coord.y) * int(tbo_size.x) + int(tbo_coord.x);
    color = texelFetch(tbo, tbo_offset).rabg;
}
)";
        d24s8_abgr_shader.Create(vs_source.c_str(), fs_source.c_str());

        OpenGLState state = OpenGLState::GetCurState();
        GLuint old_program = state.draw.shader_program;
        state.draw.shader_program = d24s8_abgr_shader.handle;
        state.Apply();

        GLint tbo_u_id = glGetUniformLocation(d24s8_abgr_shader.handle, "tbo");
        ASSERT(tbo_u_id != -1);
        glUniform1i(tbo_u_id, 0);

        state.draw.shader_program = old_program;
        state.Apply();

        d24s8_abgr_tbo_size_u_id = glGetUniformLocation(d24s8_abgr_shader.handle, "tbo_size");
        ASSERT(d24s8_abgr_tbo_size_u_id != -1);
        d24s8_abgr_viewport_u_id = glGetUniformLocation(d24s8_abgr_shader.handle, "viewport");
        ASSERT(d24s8_abgr_viewport_u_id != -1);
    }

    ~ConverterD24S8toABGR() {}

    void Convert(GLuint src_tex, const Common::Rectangle<u32>& src_rect, GLuint read_fb_handle,
                 GLuint dst_tex, const Common::Rectangle<u32>& dst_rect,
                 GLuint draw_fb_handle) override {
        OpenGLState prev_state = OpenGLState::GetCurState();
        SCOPE_EXIT({ prev_state.Apply(); });

        OpenGLState state;
        state.draw.read_framebuffer = read_fb_handle;
        state.draw.draw_framebuffer = draw_fb_handle;
        state.Apply();

        glBindBuffer(GL_PIXEL_PACK_BUFFER, d24s8_abgr_buffer.handle);

        GLsizeiptr target_pbo_size =
            static_cast<GLsizeiptr>(src_rect.GetWidth()) * src_rect.GetHeight() * 4;
        if (target_pbo_size > d24s8_abgr_buffer_size) {
            d24s8_abgr_buffer_size = target_pbo_size * 2;
            glBufferData(GL_PIXEL_PACK_BUFFER, d24s8_abgr_buffer_size, nullptr, GL_STREAM_COPY);
        }

        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                               src_tex, 0);
        glReadPixels(static_cast<GLint>(src_rect.left), static_cast<GLint>(src_rect.bottom),
                     static_cast<GLsizei>(src_rect.GetWidth()),
                     static_cast<GLsizei>(src_rect.GetHeight()), GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8, 0);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // PBO now contains src_tex in RABG format
        state.draw.shader_program = d24s8_abgr_shader.handle;
        state.draw.vertex_array = attributeless_vao.handle;
        state.viewport.x = static_cast<GLint>(dst_rect.left);
        state.viewport.y = static_cast<GLint>(dst_rect.bottom);
        state.viewport.width = static_cast<GLsizei>(dst_rect.GetWidth());
        state.viewport.height = static_cast<GLsizei>(dst_rect.GetHeight());
        state.Apply();

        OGLTexture tbo;
        tbo.Create();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, tbo.handle);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, d24s8_abgr_buffer.handle);

        glUniform2f(d24s8_abgr_tbo_size_u_id, static_cast<GLfloat>(src_rect.GetWidth()),
                    static_cast<GLfloat>(src_rect.GetHeight()));
        glUniform4f(d24s8_abgr_viewport_u_id, static_cast<GLfloat>(state.viewport.x),
                    static_cast<GLfloat>(state.viewport.y),
                    static_cast<GLfloat>(state.viewport.width),
                    static_cast<GLfloat>(state.viewport.height));

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_tex,
                               0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0,
                               0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

private:
    OGLVertexArray attributeless_vao;
    OGLBuffer d24s8_abgr_buffer;
    GLsizeiptr d24s8_abgr_buffer_size;
    OGLProgram d24s8_abgr_shader;
    GLint d24s8_abgr_tbo_size_u_id;
    GLint d24s8_abgr_viewport_u_id;
};

FormatConverterOpenGL::FormatConverterOpenGL() {
    converters[static_cast<std::size_t>(AvailableConverters::ReadPixel_D24S8_ABGR8)] =
        std::make_unique<ConverterD24S8toABGR>();
}

FormatConverterOpenGL::~FormatConverterOpenGL() = default;

static const std::map<PixelFormat, std::vector<PixelFormat>> possible_conversions{
    {PixelFormat::RGBA8, {PixelFormat::D24S8}}};

std::vector<PixelFormat> FormatConverterOpenGL::GetPossibleConversions(
    PixelFormat dst_format) const {
    auto itr = possible_conversions.find(dst_format);
    if (itr != possible_conversions.end()) {
        return itr->second;
    }
    return std::vector<PixelFormat>();
}

bool FormatConverterOpenGL::Convert(PixelFormat src_format, GLuint src_tex,
                                    const Common::Rectangle<u32>& src_rect, GLuint read_fb_handle,
                                    PixelFormat dst_format, GLuint dst_tex,
                                    const Common::Rectangle<u32>& dst_rect, GLuint draw_fb_handle) {
    auto index = FormatToIndex(src_format, dst_format);
    if (static_cast<AvailableConverters>(index) == AvailableConverters::Unavailable) {
        return false;
    }
    auto& converter = converters[index];
    converter->Convert(src_tex, src_rect, read_fb_handle, dst_tex, dst_rect, draw_fb_handle);
    return true;
}

} // namespace OpenGL
