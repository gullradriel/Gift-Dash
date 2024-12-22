/**\file states_management.h
 *  level file for hacks
 *\author Castagnier Mickaël aka Gull Ra Driel
 *\version 1.0
 *\date 29/12/2021
 */

#ifndef STATES_HEADER_FOR_HACKS
#define STATES_HEADER_FOR_HACKS

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

enum APP_KEYS {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ESC,
    KEY_SPACE,
    KEY_CTRL,
    KEY_SHIFT,
    KEY_PAD_MINUS,
    KEY_PAD_PLUS,
    KEY_PAD_ENTER,
    KEY_M,
    KEY_W,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6
};

int load_app_state(char* state_filename, long int* WIDTH, long int* HEIGHT, bool* fullscreen, char** bgmusic, double* drawFPS, double* logicFPS);

#ifdef __cplusplus
}
#endif

#endif
