#ifndef BOARD_RENDERER_HPP
#define BOARD_RENDERER_HPP

#include "asset_manager.hpp"
#include "sprite_batch.hpp"
#include "types.hpp"

#include <glm/glm.hpp>

#include <array>

class BoardRenderer {
public:
    BoardRenderer();

    bool Initialize(AssetManager* assets);

    void SetHighlight(BoardPosition pos, TileHighlight type);
    void ClearHighlights();
    void SetSelectedTile(BoardPosition pos);
    void ClearSelection();

    void RenderTileHighlights(SpriteBatch& batch);

    static glm::vec2 BoardToScreen(int board_x, int board_y);
    static glm::vec2 BoardToScreen(BoardPosition pos);
    static BoardPosition ScreenToBoard(float screen_x, float screen_y);
    static bool IsValidPosition(BoardPosition pos);

    static float GetBoardOriginX();
    static float GetBoardOriginY();

private:
    AssetManager* assets_;
    std::array<std::array<TileHighlight, BOARD_COLS>, BOARD_ROWS> highlights_;
    BoardPosition selected_tile_;
    bool has_selection_;
};

#endif
