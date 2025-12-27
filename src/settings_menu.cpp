#include "settings_menu.hpp"
#include "gpu_renderer.hpp"
#include "text_renderer.hpp"

bool g_show_settings_menu = false;

void render_settings_menu(const RenderConfig& config) {
    // Colors from docs/basic_menu_colors.md
    // Dialog body: rgba(0, 0, 47, 127)
    // Title gradient top: rgba(0, 96, 191, 127)
    // Title gradient mid: rgba(0, 0, 80, 127)
    // Title gradient bot: rgba(0, 240, 255, 127)

    // Layout proportions based on Perfect Dark reference
    float menu_width = config.window_w * 0.314271f;  // 603.4px at 1080p
    float menu_height = config.window_h * 0.75f;

    // Title bar extended 2px down (54.65px at 1080p)
    float title_height = menu_height * 0.06747f;
    float title_overhang_left = config.window_w * 0.008333f;

    // 1px gap between title bar and dialog body
    float gap = config.window_h * 0.000926f;

    // Total composition height
    float total_height = title_height + gap + menu_height;

    // Position with offset from center
    float offset_up = config.window_h * 0.042593f;
    float offset_right = config.window_w * 0.007292f;
    float left_extend = config.window_w * 0.008073f;
    float menu_y = (config.window_h - total_height) * 0.5f + title_height + gap - offset_up;
    float menu_x = (config.window_w - menu_width) * 0.5f + offset_right - left_extend;

    // 1. Draw dialog body
    float bottom_ext = config.window_h * 0.085185f;
    SDL_FRect dialog_body = {menu_x, menu_y, menu_width, menu_height + bottom_ext};
    g_gpu.draw_quad_colored(dialog_body, {0.0f, 0.0f, 47.0f/255.0f, 127.0f/255.0f});

    // 2. Draw title bar with 3-color gradient
    float title_x = menu_x - title_overhang_left;
    float title_w = config.window_w * 0.329896f;
    float title_bar_y = menu_y - title_height - gap;

    SDL_FColor top_color = {0.0f, 96.0f/255.0f, 191.0f/255.0f, 127.0f/255.0f};
    SDL_FColor mid_color = {0.0f, 0.0f, 80.0f/255.0f, 127.0f/255.0f};
    SDL_FColor bot_color = {0.0f, 240.0f/255.0f, 1.0f, 127.0f/255.0f};

    float half_h = title_height * 0.5f;
    SDL_FRect title_upper = {title_x, title_bar_y, title_w, half_h};
    g_gpu.draw_quad_gradient(title_upper, top_color, top_color, mid_color, mid_color);

    SDL_FRect title_lower = {title_x, title_bar_y + half_h, title_w, half_h};
    g_gpu.draw_quad_gradient(title_lower, mid_color, mid_color, bot_color, bot_color);

    // 3. Draw title text
    if (g_title_text.atlas) {
        float title_text_size = title_height * 0.7f + config.window_h * 0.003704f + 2.0f;
        float title_text_x = title_x + config.window_w * 0.014167f;
        float title_text_y = title_bar_y + (title_height - 1.208f * title_text_size) / 2.0f + 4.0f;
        g_title_text.draw_text("Options", title_text_x + 2, title_text_y + 2, title_text_size, {0.0f, 0.0f, 0.0f, 1.0f});
        g_title_text.draw_text("Options", title_text_x, title_text_y, title_text_size, {1.0f, 1.0f, 1.0f, 1.0f});
    }

    // 4. Draw menu items
    if (g_text.atlas) {
        float item_size = menu_height * 0.08f;
        float line_spacing = menu_height * 0.1547f;
        float item_margin = menu_width * 0.30f;
        float item_x = menu_x + item_margin + config.window_w * 0.019271f;
        float item_y = menu_y + menu_height * 0.06f - config.window_h * 0.014815f;

        SDL_FColor item_color = {0.0f, 1.0f, 1.0f, 1.0f};

        const char* menu_items[] = {
            "Audio", "Video", "Control", "Display", "Cheats", "Cinema", "Extended"
        };

        for (const char* item : menu_items) {
            g_text.draw_text(item, item_x, item_y, item_size, item_color);
            item_y += line_spacing;
        }
    }
}
