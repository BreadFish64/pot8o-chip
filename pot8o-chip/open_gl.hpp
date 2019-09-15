#pragma once

#include <cassert>
#include <glad/glad.h>
#include <fmt/format.h>
#include <fmt/printf.h>

namespace OpenGL {

constexpr char quad_source[] = R"(
#version 450
in vec2 a_pos;
out vec2 v_tex_pos;

void main() {
    v_tex_pos = a_pos;
    gl_Position = vec4(1.0 - 2.0 * a_pos, 0, 1);
})";

constexpr char bpp_frag[] = R"(
#version 450
in vec2 v_tex_pos;
out vec4 frag_color;
layout(binding=0) uniform usampler2D tex;

void main() {
    uvec4 pixel = texture(tex, v_tex_pos).rrra;
    pixel &= 1u << uint(v_tex_pos.x * 64) % 8;
    frag_color = vec4(pixel);
})";

struct Texture {
    GLuint handle = NULL;
	void Create() {
        assert(!handle);
        glGenTextures(1, &handle);
	}

	~Texture() {
        glDeleteTextures(1, &handle);
	}

	operator GLuint() {
        return handle;
	}
};

struct Buffer {
    GLuint handle = NULL;
    void Create() {
        assert(!handle);
        glGenBuffers(1, &handle);
    }

    ~Buffer() {
        glDeleteBuffers(1, &handle);
    }

    operator GLuint() {
        return handle;
    }
};

struct VAO {
    GLuint handle = NULL;
    void Create() {
        assert(!handle);
        glGenVertexArrays(1, &handle);
    }

    ~VAO() {
        glDeleteVertexArrays(1, &handle);
    }

    operator GLuint() {
        return handle;
    }
};

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

static void APIENTRY DebugHandler(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message, const void* user_param) {
        fmt::print("OpenGL {}: {}\n", id, message);
}
} // namespace OpenGL