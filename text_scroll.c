#include "text_scroll.h"

// Initialize the text manager
void init_text_manager(TextManager* tm, const char** lines, int num_lines, ALLEGRO_FONT* font, float scroll_speed, int SCREEN_HEIGHT) {
    tm->lines = lines;
    tm->num_lines = num_lines;
    tm->current_line = 0;
    tm->scroll_speed = scroll_speed;
    tm->position_y = SCREEN_HEIGHT;  // Start below the screen
    tm->font = font;
    tm->is_done = false;
}

// Render text
void render_text_manager(TextManager* tm, int SCREEN_WIDTH, int SCREEN_HEIGHT) {
    for (int i = tm->current_line; i < tm->num_lines; i++) {
        float y = tm->position_y + (i - tm->current_line) * al_get_font_line_height(tm->font);
        if (y >= SCREEN_HEIGHT || y + al_get_font_line_height(tm->font) < 0) {
            continue;  // Skip if out of screen
        }
        al_draw_text(tm->font, al_map_rgb(255, 0, 0), (SCREEN_WIDTH / 2) - 2, y - 2, ALLEGRO_ALIGN_CENTER, tm->lines[i]);
        al_draw_text(tm->font, al_map_rgb(0, 255, 0), (SCREEN_WIDTH / 2), y, ALLEGRO_ALIGN_CENTER, tm->lines[i]);
        al_draw_text(tm->font, al_map_rgb(0, 0, 255), (SCREEN_WIDTH / 2) + 2, y + 2, ALLEGRO_ALIGN_CENTER, tm->lines[i]);
    }
}

// Update text manager
void update_text_manager(TextManager* tm, float delta_time) {
    if (tm->is_done)
        return;
    tm->position_y -= tm->scroll_speed * delta_time;  // Scroll the text
                                                      // Check if the last line is fully off the screen
    if (tm->current_line == tm->num_lines - 1 &&
        tm->position_y + al_get_font_line_height(tm->font) < 0) {
        tm->is_done = true;  // Scrolling complete
    }
    if (tm->position_y + al_get_font_line_height(tm->font) < 0) {
        tm->current_line++;
        tm->position_y += al_get_font_line_height(tm->font);
        if (tm->current_line >= tm->num_lines) {
            tm->current_line = tm->num_lines - 1;  // Prevent overflow
        }
    }
}
