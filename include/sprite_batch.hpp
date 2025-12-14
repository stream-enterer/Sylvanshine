#ifndef SPRITE_BATCH_HPP
#define SPRITE_BATCH_HPP

#include "shader.hpp"

#include <glm/glm.hpp>

#include <vector>

struct SpriteVertex {
    glm::vec2 position;
    glm::vec2 tex_coord;
    glm::vec4 color;
};

struct SpriteDrawCall {
    unsigned int texture_id;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 src_rect;
    glm::vec4 color;
    float rotation;
    float z_order;
    bool flip_x;
};

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    bool Initialize();
    void Shutdown();

    void Begin(const glm::mat4& projection);
    void Draw(const SpriteDrawCall& call);
    void End();

    void SetBoardRotation(float degrees);
    void SetEntityRotation(float degrees);
    void UseEntityRotation(bool use);

private:
    Shader shader_;
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int ebo_;

    glm::mat4 projection_;
    glm::mat4 board_rotation_;
    glm::mat4 entity_rotation_;
    bool use_entity_rotation_;

    std::vector<SpriteDrawCall> draw_calls_;

    void Flush();
    void RenderSprite(const SpriteDrawCall& call);
};

#endif
