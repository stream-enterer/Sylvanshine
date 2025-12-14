#include "sprite_batch.hpp"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

static const char* VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 uProjection;
uniform mat4 uModel;

void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
)";

static const char* FRAGMENT_SHADER = R"(
#version 330 core
in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    FragColor = texColor * Color;
}
)";

SpriteBatch::SpriteBatch()
    : vao_(0), vbo_(0), ebo_(0), use_entity_rotation_(false) {
    board_rotation_ = glm::mat4(1.0f);
    entity_rotation_ = glm::mat4(1.0f);
}

SpriteBatch::~SpriteBatch() {
    Shutdown();
}

bool SpriteBatch::Initialize() {
    if (!shader_.Load(VERTEX_SHADER, FRAGMENT_SHADER)) {
        return false;
    }

    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                         (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                         (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return true;
}

void SpriteBatch::Shutdown() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    shader_.Unload();
}

void SpriteBatch::Begin(const glm::mat4& projection) {
    projection_ = projection;
    draw_calls_.clear();
}

void SpriteBatch::Draw(const SpriteDrawCall& call) {
    draw_calls_.push_back(call);
}

void SpriteBatch::End() {
    std::stable_sort(draw_calls_.begin(), draw_calls_.end(),
        [](const SpriteDrawCall& a, const SpriteDrawCall& b) {
            return a.z_order < b.z_order;
        });

    Flush();
}

void SpriteBatch::SetBoardRotation(float degrees) {
    board_rotation_ = glm::rotate(glm::mat4(1.0f), glm::radians(degrees),
                                  glm::vec3(1.0f, 0.0f, 0.0f));
}

void SpriteBatch::SetEntityRotation(float degrees) {
    entity_rotation_ = glm::rotate(glm::mat4(1.0f), glm::radians(degrees),
                                   glm::vec3(1.0f, 0.0f, 0.0f));
}

void SpriteBatch::UseEntityRotation(bool use) {
    use_entity_rotation_ = use;
}

void SpriteBatch::Flush() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_.Use();
    shader_.SetMat4("uProjection", projection_);
    shader_.SetInt("uTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_);

    for (const auto& call : draw_calls_) {
        RenderSprite(call);
    }

    glBindVertexArray(0);
}

void SpriteBatch::RenderSprite(const SpriteDrawCall& call) {
    glBindTexture(GL_TEXTURE_2D, call.texture_id);

    float u1 = call.src_rect.x;
    float v1 = call.src_rect.y;
    float u2 = call.src_rect.x + call.src_rect.z;
    float v2 = call.src_rect.y + call.src_rect.w;

    if (call.flip_x) {
        std::swap(u1, u2);
    }

    float vertices[] = {
        0.0f, 0.0f, u1, v1, call.color.r, call.color.g, call.color.b, call.color.a,
        1.0f, 0.0f, u2, v1, call.color.r, call.color.g, call.color.b, call.color.a,
        1.0f, 1.0f, u2, v2, call.color.r, call.color.g, call.color.b, call.color.a,
        0.0f, 1.0f, u1, v2, call.color.r, call.color.g, call.color.b, call.color.a
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(call.position.x - call.size.x * 0.5f,
                                             call.position.y - call.size.y * 0.5f,
                                             0.0f));
    model = glm::scale(model, glm::vec3(call.size.x, call.size.y, 1.0f));

    shader_.SetMat4("uModel", model);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
