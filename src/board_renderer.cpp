#include "board_renderer.hpp"
#include <GL/glew.h>

#include <cmath>

BoardRenderer::BoardRenderer() : assets_(nullptr), has_selection_(false) {
    ClearHighlights();
    selected_tile_ = {-1, -1};
}

bool BoardRenderer::Initialize(AssetManager* assets) {
    assets_ = assets;
    return assets_ != nullptr;
}

void BoardRenderer::SetHighlight(BoardPosition pos, TileHighlight type) {
    if (IsValidPosition(pos)) {
        highlights_[pos.y][pos.x] = type;
    }
}

void BoardRenderer::ClearHighlights() {
    for (int y = 0; y < BOARD_ROWS; ++y) {
        for (int x = 0; x < BOARD_COLS; ++x) {
            highlights_[y][x] = TileHighlight::NONE;
        }
    }
}

void BoardRenderer::SetSelectedTile(BoardPosition pos) {
    if (IsValidPosition(pos)) {
        selected_tile_ = pos;
        has_selection_ = true;
    }
}

void BoardRenderer::ClearSelection() {
    has_selection_ = false;
    selected_tile_ = {-1, -1};
}

void BoardRenderer::RenderTileHighlights(SpriteBatch& batch) {
    if (!assets_) return;

    unsigned int action_tex = assets_->GetTilesActionTexture();
    if (action_tex == 0) return;

    for (int y = 0; y < BOARD_ROWS; ++y) {
        for (int x = 0; x < BOARD_COLS; ++x) {
            TileHighlight type = highlights_[y][x];
            if (type == TileHighlight::NONE) continue;

            glm::vec2 pos = BoardToScreen(x, y);

            SpriteDrawCall call;
            call.texture_id = action_tex;
            call.position = pos;
            call.size = glm::vec2(TILE_SIZE, TILE_SIZE);
            call.src_rect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

            float opacity = TILE_DIM_OPACITY / 255.0f;
            glm::vec3 color(1.0f);

            switch (type) {
                case TileHighlight::MOVE:
                    opacity = TILE_HOVER_OPACITY / 255.0f;
                    color = glm::vec3(1.0f, 1.0f, 1.0f);
                    break;
                case TileHighlight::ATTACK:
                    opacity = TILE_SELECT_OPACITY / 255.0f;
                    color = glm::vec3(1.0f, 0.39f, 0.39f);
                    break;
                case TileHighlight::SELECTED:
                    opacity = TILE_SELECT_OPACITY / 255.0f;
                    color = glm::vec3(0.5f, 1.0f, 0.5f);
                    break;
                default:
                    break;
            }

            call.color = glm::vec4(color, opacity);
            call.rotation = 0.0f;
            call.z_order = 1.0f;
            call.flip_x = false;

            batch.Draw(call);
        }
    }
}

float BoardRenderer::GetBoardOriginX() {
    return (static_cast<float>(WINDOW_WIDTH) - BOARD_COLS * TILE_SIZE) * 0.5f +
           TILE_SIZE * 0.5f + TILE_OFFSET_X;
}

float BoardRenderer::GetBoardOriginY() {
    return (static_cast<float>(WINDOW_HEIGHT) - BOARD_ROWS * TILE_SIZE) * 0.5f +
           TILE_SIZE * 0.5f + TILE_OFFSET_Y;
}

glm::vec2 BoardRenderer::BoardToScreen(int board_x, int board_y) {
    float origin_x = GetBoardOriginX();
    float origin_y = GetBoardOriginY();

    return glm::vec2(
        board_x * TILE_SIZE + origin_x,
        board_y * TILE_SIZE + origin_y
    );
}

glm::vec2 BoardRenderer::BoardToScreen(BoardPosition pos) {
    return BoardToScreen(pos.x, pos.y);
}

BoardPosition BoardRenderer::ScreenToBoard(float screen_x, float screen_y) {
    float origin_x = GetBoardOriginX();
    float origin_y = GetBoardOriginY();

    int board_x = static_cast<int>(std::floor((screen_x - origin_x + TILE_SIZE * 0.5f) / TILE_SIZE));
    int board_y = static_cast<int>(std::floor((screen_y - origin_y + TILE_SIZE * 0.5f) / TILE_SIZE));

    return {board_x, board_y};
}

bool BoardRenderer::IsValidPosition(BoardPosition pos) {
    return pos.x >= 0 && pos.x < BOARD_COLS && pos.y >= 0 && pos.y < BOARD_ROWS;
}
