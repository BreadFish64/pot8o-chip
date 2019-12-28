#pragma once

#include <cassert>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <glad/glad.h>

namespace OpenGL {

constexpr char quad_source[] = R"(
#version 450
in vec2 vertex_pos;
out vec2 tex_coord;

void main() {
    tex_coord = vertex_pos / 2 + .5;
    gl_Position = vec4(vertex_pos * -1, 0, 1);
})";

constexpr char bpp_frag[] = R"(
#version 450
in vec2 tex_coord;
out vec4 frag_color;
layout(binding=0) uniform usampler2D tex;

void main() {
    uvec4 pixel = texture(tex, tex_coord).rrrr;
    pixel &= 1u << (uint(tex_coord.x * 64) & 7);
    frag_color = vec4(pixel);
})";

#define GL_RESOURCE_WRAPPER(_resource)                                                             \
    struct _resource {                                                                             \
        GLuint handle = NULL;                                                                      \
        void Create() {                                                                            \
            assert(!handle);                                                                       \
            glGen##_resource##s(1, &handle);                                                       \
        }                                                                                          \
                                                                                                   \
        ~_resource() {                                                                             \
            glDelete##_resource##s(1, &handle);                                                    \
        }                                                                                          \
                                                                                                   \
        operator GLuint() {                                                                        \
            return handle;                                                                         \
        }                                                                                          \
    }

GL_RESOURCE_WRAPPER(VertexArray);
GL_RESOURCE_WRAPPER(Buffer);
GL_RESOURCE_WRAPPER(Texture);

struct Program {
    GLuint vs_handle = NULL, fs_handle = NULL, prog_handle = NULL;

    bool Create(std::vector<const GLchar*>&& vs_source, std::vector<const GLchar*>&& fs_source) {
        assert(!(prog_handle | vs_handle | fs_handle));
        vs_handle = glCreateShader(GL_VERTEX_SHADER);
        fs_handle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vs_handle, vs_source.size(), vs_source.data(), NULL);
        glShaderSource(fs_handle, fs_source.size(), fs_source.data(), NULL);
        glCompileShader(vs_handle);
        glCompileShader(fs_handle);
        for (GLuint shader : {vs_handle, fs_handle}) {
            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE) {
                GLint length = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
                std::vector<GLchar> error_log(length);
                glGetShaderInfoLog(shader, length, &length, error_log.data());
                fmt::print("Shader Compilation Failed:\n{}\n", error_log.data());
                glDeleteShader(shader);
                return false;
            }
        }

        prog_handle = glCreateProgram();
        glAttachShader(prog_handle, vs_handle);
        glAttachShader(prog_handle, fs_handle);
        glLinkProgram(prog_handle);

        GLint isLinked = 0;
        glGetProgramiv(prog_handle, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE) {
            GLint length = 0;
            glGetProgramiv(prog_handle, GL_INFO_LOG_LENGTH, &length);
            std::vector<GLchar> error_log(length);
            glGetProgramInfoLog(prog_handle, length, &length, error_log.data());
            fmt::print("Shader Compilation Failed:\n{}\n", error_log.data());
            glDeleteProgram(prog_handle);
            glDeleteShader(vs_handle);
            glDeleteShader(fs_handle);
            return false;
        }

        glDetachShader(prog_handle, vs_handle);
        glDetachShader(prog_handle, fs_handle);
        return true;
    }

    ~Program() {
        glDeleteShader(vs_handle);
        glDeleteShader(fs_handle);
        glDeleteProgram(prog_handle);
    }

    operator GLuint() {
        return prog_handle;
    }
};

#undef GL_RESOURCE_WRAPPER

static void APIENTRY DebugHandler(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const void* user_param) {
    fmt::print("OpenGL {}: {}\n", id, message);
}
} // namespace OpenGL