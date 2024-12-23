/**\file text_scroll.h
 *  text scroll for krampus hack
 *\author Castagnier Mickaël aka Gull Ra Driel
 *\version 1.0
 *\date 23/12/2024
 */

#ifndef TEXT_HEADER_FOR_KRAMPUSHACK
#define TEXT_HEADER_FOR_KRAMPUSHACK

#ifdef __cplusplus
extern "C" {
#endif

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>

// Define a simple text manager structure
typedef struct {
    const char** lines;  // Array of strings to display
    int num_lines;       // Total number of lines
    int current_line;    // Current line to display
    float scroll_speed;  // Speed of scrolling
    float position_y;    // Current Y position
    ALLEGRO_FONT* font;  // Font to render text
    bool is_done;        // Flag to indicate scrolling is complete
} TextManager;

void init_text_manager(TextManager* tm, const char** lines, int num_lines, ALLEGRO_FONT* font, float scroll_speed, int SCREEN_HEIGHT);
void render_text_manager(TextManager* tm, int SCREEN_WIDTH, int SCREEN_HEIGHT);
void update_text_manager(TextManager* tm, float delta_time);

#ifdef __cplusplus
}
#endif

#endif
