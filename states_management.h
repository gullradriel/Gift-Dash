/**\file states_management.h
 *
 *  level file for hacks
 *
 *\author Castagnier Mickaël aka Gull Ra Driel
 *
 *\version 1.0
 *
 *\date 29/12/2021 
 *
 */



#ifndef STATES_HEADER_FOR_HACKS
#define STATES_HEADER_FOR_HACKS

#ifdef __cplusplus
extern "C" {
#endif

    int load_app_state( char *state_filename , size_t *WIDTH , size_t *HEIGHT , bool *fullscreen , char **bgmusic , double *drawFPS , double *logicFPS );

#ifdef __cplusplus
}
#endif

#endif
