#ifndef _RWENGINE_OPENGLRENDERER_HPP_
#define _RWENGINE_OPENGLRENDERER_HPP_

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <array>

#include <glm/glm.hpp>

#include <gl/gl_core_3_3.h>
#include <gl/GeometryBuffer.hpp>

class DrawBuffer;

typedef uint64_t RenderKey;

// Maximum depth of debug group stack
#define MAX_DEBUG_DEPTH 5

typedef std::uint32_t RenderIndex;

struct VertexP2 {
    glm::vec2 position{};

    static const AttributeList vertex_attributes() {
        return {{ATRS_Position, 2, sizeof(VertexP2), 0ul}};
    }

    VertexP2(float _x, float _y)
        : position({_x, _y}) {
    }

    VertexP2() = default;
};


struct VertexP3 {
    glm::vec3 position{};

    static const AttributeList vertex_attributes() {
        return {
            {ATRS_Position, 3, sizeof(VertexP3), 0ul},
        };
    }

    VertexP3(float _x, float _y, float _z)
        : position({_x, _y, _z}) {
    }

    VertexP3() = default;
};

/**
 * Enum used to determine which blending mode to use
 */
enum class BlendMode {
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_ADDITIVE
};

enum class DepthMode {
    OFF,
    LESS,
};

class Renderer {
public:
    typedef std::array<GLuint,2> Textures;

    /**
     * @brief The DrawParameters struct stores drawing state
     *
     * The state for texture units, blending and material properties
     * are received for drawing through this structure.
     *
     * Since not all draws use the same shaders, material properties
     * should be controlled via a different mechanism.
     */
    struct DrawParameters {
        /// Number of indices
        size_t count{};
        /// Start index.
        size_t start{};
        /// Textures to use
        Textures textures{};
        /// Blending mode
        BlendMode blendMode = BlendMode::BLEND_NONE;
        /// Depth
        DepthMode depthMode = DepthMode::LESS;
        /// Depth writing state
        bool depthWrite = true;
        /// Material
        glm::u8vec4 colour{};
        /// Material
        float ambient{1.f};
        /// Material
        float diffuse{1.f};
        /// Material
        float visibility{1.f};

        // Default state -- should be moved to materials
        DrawParameters() = default;
    };

    /**
     * @brief The RenderInstruction struct Generic Rendering instruction
     *
     * These are generated by the ObjectRenderer, and passed in to the
     * OpenGLRenderer by GameRenderer.
     */
    struct RenderInstruction {
        RenderKey sortKey;
        // Ideally, this would just be an index into a buffer that contains the
        // matrix
        glm::mat4 model;
        DrawBuffer* dbuff;
        Renderer::DrawParameters drawInfo;

        RenderInstruction(RenderKey key, const glm::mat4& model,
                          DrawBuffer* dbuff, const Renderer::DrawParameters& dp)
            : sortKey(key), model(model), dbuff(dbuff), drawInfo(dp) {
        }
    };
    typedef std::vector<RenderInstruction> RenderList;

    struct ObjectUniformData {
        glm::mat4 model{1.0f};
        glm::vec4 colour{1.0f};
        float diffuse{};
        float ambient{};
        float visibility{};
    };

    struct SceneUniformData {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::vec4 ambient{};
        glm::vec4 dynamic{};
        glm::vec4 fogColour{};
        glm::vec4 campos{};
        float fogStart{};
        float fogEnd{};
    };

    class ShaderProgram {
        // This just provides an opaque handle for external users.
        public:
            virtual ~ShaderProgram() = 0;
    };

    virtual ~Renderer() = default;

    virtual std::string getIDString() const = 0;

    virtual std::unique_ptr<ShaderProgram> createShader(const std::string& vert,
                                        const std::string& frag) = 0;

    virtual void useProgram(ShaderProgram* p) = 0;

    /// @todo dont use GLint in the interface.
    virtual void setProgramBlockBinding(ShaderProgram* p,
                                        const std::string& name,
                                        GLint point) = 0;
    virtual void setUniformTexture(ShaderProgram* p, const std::string& name,
                                   GLint tex) = 0;
    virtual void setUniform(ShaderProgram* p, const std::string& name,
                            const glm::mat4& m) = 0;
    virtual void setUniform(ShaderProgram* p, const std::string& name,
                            const glm::vec4& v) = 0;
    virtual void setUniform(ShaderProgram* p, const std::string& name,
                            const glm::vec3& v) = 0;
    virtual void setUniform(ShaderProgram* p, const std::string& name,
                            const glm::vec2& v) = 0;
    virtual void setUniform(ShaderProgram* p, const std::string& name,
                            float f) = 0;

    virtual void clear(const glm::vec4& colour, bool clearColour = true,
                       bool clearDepth = true) = 0;

    virtual void setSceneParameters(const SceneUniformData& data) = 0;

    virtual void draw(const glm::mat4& model, DrawBuffer* draw,
                      const DrawParameters& p) = 0;
    virtual void drawArrays(const glm::mat4& model, DrawBuffer* draw,
                            const DrawParameters& p) = 0;

    virtual void drawBatched(const RenderList& list) = 0;

    void setViewport(const glm::ivec2& vp);
    const glm::ivec2& getViewport() const {
        return viewport;
    }

    const glm::mat4& get2DProjection() const {
        return projection2D;
    }

    virtual void invalidate() = 0;

    /**
     * Resets all per-frame counters.
     */
    void swap();

    /**
     * Returns the number of draw calls issued for the current frame.
     */
    int getDrawCount();
    int getTextureCount();
    int getBufferCount();

    const SceneUniformData& getSceneData() const;

    /**
     * Profiling data returned by popDebugGroup.
     * Not all fields will be populated, depending on
     * USING(RENDER_PROFILER)
     */
    struct ProfileInfo {
        GLuint64 timerStart{};
        GLuint64 duration{};
        unsigned int primitives{};
        unsigned int draws{};
        unsigned int textures{};
        unsigned int buffers{};
        unsigned int uploads{};
    };

    /**
     * Signals the start of a debug group
     */
    virtual void pushDebugGroup(const std::string& title) = 0;
    /**
     * Ends the current debug group and returns the profiling information
     * for that group. The returned value is valid until the next call to
     * pushDebugGroup
     */
    virtual const ProfileInfo& popDebugGroup() = 0;

private:
    glm::ivec2 viewport{};
    glm::mat4 projection2D{1.0f};

protected:
    int drawCounter{};
    int textureCounter{};
    int bufferCounter{};
    SceneUniformData lastSceneData{};
};

class OpenGLRenderer final : public Renderer {
public:
    class OpenGLShaderProgram final : public ShaderProgram {
        GLuint program;
        std::map<std::string, GLint> uniforms;

    public:
        OpenGLShaderProgram(GLuint p) : program(p) {
        }

        ~OpenGLShaderProgram() override {
            glDeleteProgram(program);
        }

        GLuint getName() const {
            return program;
        }

        GLint getUniformLocation(const std::string& name) {
            auto c = uniforms.find(name.c_str());
            GLint loc = -1;
            if (c == uniforms.end()) {
                loc = glGetUniformLocation(program, name.c_str());
                uniforms[name] = loc;
            } else {
                loc = c->second;
            }
            return loc;
        }
    };

    OpenGLRenderer();

    ~OpenGLRenderer() override = default;

    std::string getIDString() const override;

    std::unique_ptr<ShaderProgram> createShader(const std::string& vert,
                                const std::string& frag) override;
    void setProgramBlockBinding(ShaderProgram* p, const std::string& name,
                                GLint point) override;
    void setUniformTexture(ShaderProgram* p, const std::string& name,
                           GLint tex) override;
    void setUniform(ShaderProgram* p, const std::string& name,
                    const glm::mat4& m) override;
    void setUniform(ShaderProgram* p, const std::string& name,
                    const glm::vec4& m) override;
    void setUniform(ShaderProgram* p, const std::string& name,
                    const glm::vec3& m) override;
    void setUniform(ShaderProgram* p, const std::string& name,
                    const glm::vec2& m) override;
    void setUniform(ShaderProgram* p, const std::string& name,
                    float f) override;
    void useProgram(ShaderProgram* p) override;

    void clear(const glm::vec4& colour, bool clearColour,
               bool clearDepth) override;

    void setSceneParameters(const SceneUniformData& data) override;

    void setDrawState(const glm::mat4& model, DrawBuffer* draw,
                      const DrawParameters& p);

    void draw(const glm::mat4& model, DrawBuffer* draw,
              const DrawParameters& p) override;
    void drawArrays(const glm::mat4& model, DrawBuffer* draw,
                    const DrawParameters& p) override;

    void drawBatched(const RenderList& list) override;

    void invalidate() override;

    void pushDebugGroup(const std::string& title) override;

    const ProfileInfo& popDebugGroup() override;

private:
    struct Buffer {
        GLuint name{};
        GLuint currentEntry{};

        GLuint entryCount{};
        GLuint entrySize{};
        GLsizei bufferSize{};
    };

    void useDrawBuffer(DrawBuffer* dbuff);

    void useTexture(GLuint unit, GLuint tex);

    Buffer UBOObject {};
    Buffer UBOScene {};

    // State Cache
    DrawBuffer* currentDbuff = nullptr;
    OpenGLShaderProgram* currentProgram = nullptr;
    BlendMode blendMode = BlendMode::BLEND_NONE;
    DepthMode depthMode = DepthMode::OFF;
    bool depthWriteEnabled = false;
    GLuint currentUBO = 0;
    GLuint currentUnit = 0;
    std::map<GLuint, GLuint> currentTextures;

    // Set state
    void setBlend(BlendMode mode) {
        if (mode!=BlendMode::BLEND_NONE && blendMode==BlendMode::BLEND_NONE)//To don't call glEnable again when it already enabled
            glEnable(GL_BLEND);

        if (mode!=blendMode) {
            switch (mode) {
                default:
                    assert(false);
                    break;
                case BlendMode::BLEND_NONE:
                    glDisable(GL_BLEND);
                    break;
                case BlendMode::BLEND_ALPHA:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case BlendMode::BLEND_ADDITIVE:
                    glBlendFunc(GL_ONE, GL_ONE);
                    break;

            }
        }

        blendMode = mode;
    }

    void setDepthMode(DepthMode mode) {
        if (mode != depthMode) {
            if (depthMode == DepthMode::OFF) glEnable(GL_DEPTH_TEST);
            switch(mode) {
                case DepthMode::OFF: glDisable(GL_DEPTH_TEST); break;
                case DepthMode::LESS: glDepthFunc(GL_LESS); break;
            }
            depthMode = mode;
        }
    }

    void setDepthWrite(bool enable) {
        if (enable != depthWriteEnabled) {
            glDepthMask(enable ? GL_TRUE : GL_FALSE);
            depthWriteEnabled = enable;
        }
    }

    template <class T>
    void uploadUBO(Buffer& buffer, const T& data) {
        uploadUBOEntry(buffer, &data, sizeof(T));
#ifdef RW_GRAPHICS_STATS
        if (currentDebugDepth > 0) {
            profileInfo[currentDebugDepth - 1].uploads++;
        }
#endif
    }

    // Buffer Helpers
    bool createUBO(Buffer& out, GLsizei size, GLsizei entrySize);

    void attachUBO(GLuint buffer) {
        if (currentUBO != buffer) {
            glBindBuffer(GL_UNIFORM_BUFFER, buffer);
            currentUBO = buffer;
        }
    }

    void uploadUBOEntry(Buffer& buffer, const void *data, size_t size);

    // Debug group profiling timers
    ProfileInfo profileInfo[MAX_DEBUG_DEPTH];
    GLuint debugQuery;
#ifdef RW_GRAPHICS_STATS
    int currentDebugDepth = 0;
#endif
};

/// @todo remove these from here
GLuint compileShader(GLenum type, const char* source);
GLuint compileProgram(const char* vertex, const char* fragment);

typedef Renderer::RenderList RenderList;

#endif
