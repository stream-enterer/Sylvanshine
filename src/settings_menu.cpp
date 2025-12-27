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

    // Menu content
    const char* menu_items[] = {
        "Audio", "Video", "Control", "Display", "Cheats", "Cinema", "Extended"
    };

    // Text sizing
    float item_size = config.window_h * 0.06f;
    float line_spacing = item_size * 1.934f;
    size_t num_items = sizeof(menu_items) / sizeof(menu_items[0]);

    // Dynamic width based on content (fixed margins)
    float left_margin = config.window_w * 0.1135f;   // 218px at 1920w
    float right_margin = config.window_w * 0.04583f; // 88px at 1920w

    float menu_width;
    if (g_text.atlas) {
        float max_text_width = 0;
        for (const char* item : menu_items) {
            max_text_width = std::max(max_text_width, g_text.measure_width(item, item_size));
        }
        menu_width = left_margin + max_text_width + right_margin;
    } else {
        menu_width = config.window_w * 0.314271f;  // fallback if font not loaded
    }

    // Dynamic height based on content (fixed margins)
    // Note: top_margin is to text bounding box; subtract ~19px font padding for visual gap
    float top_margin = config.window_h * 0.02315f;   // 25px at 1080p → 44px visual gap
    float bottom_margin = config.window_h * 0.04074f; // 44px at 1080p
    float content_height = (num_items - 1) * line_spacing + item_size;
    float menu_height = top_margin + content_height + bottom_margin;

    // Title bar (decoupled from menu_height)
    float title_height = config.window_h * 0.0506f;  // 54.65px at 1080p
    float title_overhang_left = config.window_w * 0.008333f;

    // 1px gap between title bar and dialog body
    float gap = config.window_h * 0.000926f;

    // Total composition height (title + gap + dialog body)
    float total_height = title_height + gap + menu_height;

    // Center entire composition on screen
    float composition_y = (config.window_h - total_height) * 0.5f;
    float menu_x = (config.window_w - menu_width) * 0.5f;
    float menu_y = composition_y + title_height + gap;

    // 1. Draw dialog body
    SDL_FRect dialog_body = {menu_x, menu_y, menu_width, menu_height};
    g_gpu.draw_quad_colored(dialog_body, {0.0f, 0.0f, 47.0f/255.0f, 127.0f/255.0f});

    // 2. Draw title bar with 3-color gradient
    float title_x = menu_x - title_overhang_left;
    float title_overhang_right = config.window_w * 0.00729f;  // 14px at 1920w
    float title_w = menu_width + title_overhang_left + title_overhang_right;
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
        float item_x = menu_x + left_margin;
        float item_y = menu_y + top_margin;

        SDL_FColor item_color = {0.0f, 1.0f, 1.0f, 1.0f};

        for (const char* item : menu_items) {
            g_text.draw_text(item, item_x, item_y, item_size, item_color);
            item_y += line_spacing;
        }
    }
}
