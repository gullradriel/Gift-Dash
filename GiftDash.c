/**\file GiftDash.c
 *  GiftDash Main File
 *\author Castagnier Mickaël
 *\version 1.0
 *\date 30/12/2021
 */

#include <locale.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"

#include "nilorea/n_common.h"
#include "nilorea/n_list.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"
#include "nilorea/n_thread_pool.h"
#include "nilorea/n_time.h"
#include "nilorea/n_particles.h"

#include "sledge_physics.h"
#include "states_management.h"

#define RESERVED_SAMPLES 16
#define MAX_SAMPLE_DATA 10

/******************************************************************************
 *                           VARIOUS DECLARATIONS                             *
 ******************************************************************************/

int DONE = 0,            /* Flag to check if we are always running */
    getoptret = 0,       /* launch parameter check */
    log_level = LOG_ERR; /* default LOG_LEVEL */

double drawFPS = 60.0;
double logicFPS = 240.0;

ALLEGRO_DISPLAY* display = NULL;
ALLEGRO_TIMER* fps_timer = NULL;
ALLEGRO_TIMER* logic_timer = NULL;
ALLEGRO_SAMPLE* sample_data[MAX_SAMPLE_DATA] = {NULL};
ALLEGRO_EVENT_QUEUE* event_queue = NULL;

N_TIME logic_chrono;
N_TIME drawing_chrono;

long int WIDTH = 1280, HEIGHT = 800;
bool fullscreen = 0;
char* bgmusic = NULL;

THREAD_POOL* thread_pool = NULL;

// Test rectangle
CollisionRectangle testRect = {300, 250, 100, 100};
size_t logic_duration = 0;
size_t drawing_duration = 0;
bool collision = false;

bool backbuffer = 1;
ALLEGRO_BITMAP* png_good = NULL;
ALLEGRO_BITMAP* png_evil = NULL;
ALLEGRO_BITMAP* scrbuf = NULL;
ALLEGRO_BITMAP* bitmap = NULL;
ALLEGRO_BITMAP* christmasPresents[16];
ALLEGRO_BITMAP* bogeymanPresents[16];

bool do_draw = 1, do_logic = 1;
int mx = 0, my = 0, mouse_button = 0, mouse_b1 = 0, mouse_b2 = 0;

int key[19] = {false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false};

ALLEGRO_BITMAP* santaSledgebmp = NULL;
VEHICLE santaSledge = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
PARTICLE_SYSTEM* particle_system = NULL;

enum {
    good,
    evil };

typedef struct gift_dash_object
{
    long int x;
    long int y;
    int type;
    int id;
    int w;
    int h;
}gift_dash_object;

LIST *good_presents = NULL;
LIST *bad_presents = NULL;

double tx = 0, ty = 0;

int check_item_collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    // Check if one box is to the left, right, above, or below the other
    if (x1 + w1 <= x2 || x1 >= x2 + w2 || y1 + h1 <= y2 || y1 >= y2 + h2) {
        return 0; // No overlap
    }
    return 1; // Overlap
}

int main(int argc, char* argv[]) {
    /* Set the locale to the POSIX C environment */
    setlocale(LC_ALL, "POSIX");

    /*
     * INITIALISATION
     */
    set_log_level(LOG_NOTICE);

    if (load_app_state("app_config.json", &WIDTH, &HEIGHT, &fullscreen, &bgmusic,
                &drawFPS, &logicFPS) != TRUE) {
        n_log(LOG_ERR, "couldn't load app_config.json !");
        exit(1);
    }
    n_log(LOG_DEBUG, "%s starting with params: %dx%d fullscreen(%d), music: %s",
            argv[0], WIDTH, HEIGHT, fullscreen, _str(bgmusic));

    N_STR* log_file = NULL;
    nstrprintf(log_file, "%s.log", argv[0]);
    /*set_log_file( _nstr( log_file ) );*/
    free_nstr(&log_file);

    char ver_str[128] = "";

    while ((getoptret = getopt(argc, argv, "hvV:L:")) != EOF) {
        switch (getoptret) {
            case 'h':
                n_log(LOG_NOTICE,
                        "\n    %s -h help -v version -V DEBUGLEVEL "
                        "(NOLOG,VERBOSE,NOTICE,ERROR,DEBUG)\n",
                        argv[0]);
                exit(TRUE);
            case 'v':
                sprintf(ver_str, "%s %s", __DATE__, __TIME__);
                exit(TRUE);
                break;
            case 'V':
                if (!strncmp("INFO", optarg, 6)) {
                    log_level = LOG_INFO;
                } else {
                    if (!strncmp("NOTICE", optarg, 6)) {
                        log_level = LOG_NOTICE;
                    } else {
                        if (!strncmp("VERBOSE", optarg, 7)) {
                            log_level = LOG_NOTICE;
                        } else {
                            if (!strncmp("ERROR", optarg, 5)) {
                                log_level = LOG_ERR;
                            } else {
                                if (!strncmp("DEBUG", optarg, 5)) {
                                    log_level = LOG_DEBUG;
                                } else {
                                    n_log(LOG_ERR, "%s is not a valid log level", optarg);
                                    exit(FALSE);
                                }
                            }
                        }
                    }
                }
                n_log(LOG_NOTICE, "LOG LEVEL UP TO: %d", log_level);
                set_log_level(log_level);
                break;
            case 'L':
                n_log(LOG_NOTICE, "LOG FILE: %s", optarg);
                set_log_file(optarg);
                break;
            case '?': {
                          switch (optopt) {
                              case 'V':
                                  n_log(LOG_ERR,
                                          "\nPlease specify a log level after -V. \nAvailable "
                                          "values: NOLOG,VERBOSE,NOTICE,ERROR,DEBUG");
                                  break;
                              case 'L':
                                  n_log(LOG_ERR, "\nPlease specify a log file after -L");
                              default:
                                  break;
                          }
                      }
                      __attribute__((fallthrough));
            default:
                      n_log(LOG_ERR,
                              "\n    %s -h help -v version -V DEBUGLEVEL "
                              "(NOLOG,VERBOSE,NOTICE,ERROR,DEBUG) -L logfile",
                              argv[0]);
                      exit(FALSE);
        }
    }

    /* allegro 5 + addons loading */
    if (!al_init()) {
        n_abort("Could not init Allegro.\n");
    }
    if (!al_init_acodec_addon()) {
        n_abort("Could not register addons.\n");
    }
    if (!al_install_audio()) {
        n_abort("Unable to initialize audio addon\n");
    }
    if (!al_init_acodec_addon()) {
        n_abort("Unable to initialize acoded addon\n");
    }
    if (!al_init_image_addon()) {
        n_abort("Unable to initialize image addon\n");
    }
    if (!al_init_primitives_addon()) {
        n_abort("Unable to initialize primitives addon\n");
    }
    if (!al_init_font_addon()) {
        n_abort("Unable to initialize font addon\n");
    }
    if (!al_init_ttf_addon()) {
        n_abort("Unable to initialize ttf_font addon\n");
    }
    if (!al_install_keyboard()) {
        n_abort("Unable to initialize keyboard handler\n");
    }
    if (!al_install_mouse()) {
        n_abort("Unable to initialize mouse handler\n");
    }

    if (!al_reserve_samples(RESERVED_SAMPLES)) {
        n_abort("Could not set up voice and mixer.\n");
    }

    memset(sample_data, 0, sizeof(sample_data));

    event_queue = al_create_event_queue();
    if (!event_queue) {
        fprintf(stderr, "failed to create event_queue!\n");
        return -1;
    }

    if (fullscreen) {
        al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN_WINDOW);
    } else {
        al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_WINDOWED);
    }

    // it's not working under linux. I don't know why.
    // al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP|ALLEGRO_NO_PRESERVE_TEXTURE );

    display = al_create_display(WIDTH, HEIGHT);
    if (!display) {
        n_abort("Unable to create display\n");
    }

    al_set_window_title(display, "GiftDash");

    ALLEGRO_FONT* font = al_load_font("DATA/2Dumb.ttf", 18, 0);

    fps_timer = al_create_timer(1.0 / drawFPS);
    logic_timer = al_create_timer(1.0 / logicFPS);
    al_start_timer(fps_timer);
    al_start_timer(logic_timer);
    al_register_event_source(event_queue, al_get_timer_event_source(fps_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));

    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    al_register_event_source(event_queue, al_get_display_event_source(display));

    bitmap = al_create_bitmap(WIDTH, HEIGHT);

    al_hide_mouse_cursor(display);

    int GRID_SIZE = 3 ;
    int ICON_SIZE = 84 ;

    // Load the PNG file containing the christmas icons
    png_good = al_load_bitmap("DATA/Gfxs/ChristmasIcons.png");
    if (!png_good) {
        fprintf(stderr, "Failed to load ChristmasIcons PNG file.\n");
        return -1;
    }
    // Loop through the grid (4x4) and extract each icon (64x64 pixels)
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Calculate the position of the current icon in the PNG
            int x = j * ICON_SIZE;
            int y = i * ICON_SIZE;

            // Extract the icon and store it in the iconlist array
            christmasPresents[i * GRID_SIZE + j] = al_create_sub_bitmap(png_good, x, y, ICON_SIZE, ICON_SIZE);
            if (!christmasPresents[i * GRID_SIZE + j]) {
                fprintf(stderr, "Failed to create sub bitmap.\n");
                return -1;
            }
        }
    }
    // init good presents LIST
    good_presents = new_generic_list( -1 );
    for( int it = 0 ; it < 100 ; it ++ )
    {
        gift_dash_object *object = NULL ;
        Malloc( object , gift_dash_object , 1 );
        object -> x = (-3 * WIDTH) + rand()%((6*WIDTH)-64);
        object -> y = (-3 * HEIGHT) + rand()%((6*HEIGHT)-64);
        object -> type = good ;
        object -> id = rand()%(GRID_SIZE*GRID_SIZE);
        object -> w = ICON_SIZE;
        object -> h = ICON_SIZE;
        list_push( good_presents , object , free );
    }

    // Load the PNG file containing the bogeyman icons
    GRID_SIZE = 4 ;
    ICON_SIZE = 128 ;
    png_evil = al_load_bitmap("DATA/Gfxs/BogeymanIcons.png");
    if (!png_evil) {
        fprintf(stderr, "Failed to load BogeymanIcons PNG file.\n");
        return -1;
    }
    // Loop through the grid (4x4) and extract each icon (64x64 pixels)
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            // Calculate the position of the current icon in the PNG
            int x = j * ICON_SIZE;
            int y = i * ICON_SIZE;

            // Extract the icon and store it in the iconlist array
            bogeymanPresents[i * GRID_SIZE + j] = al_create_sub_bitmap(png_evil, x, y, ICON_SIZE, ICON_SIZE);
            if (!bogeymanPresents[i * GRID_SIZE + j]) {
                fprintf(stderr, "Failed to create sub bitmap.\n");
                return -1;
            }
        }
    }
    // init bad presents LIST
    bad_presents = new_generic_list( -1 );
    for( int it = 0 ; it < 100 ; it ++ )
    {
        gift_dash_object *object = NULL ;
        Malloc( object , gift_dash_object , 1 );
        object -> type = evil ;
        object -> id = rand()%(GRID_SIZE*GRID_SIZE);
        object -> w = ICON_SIZE;
        object -> h = ICON_SIZE;
        object -> x = (-3 * WIDTH) + rand()%((6*WIDTH)-64);
        object -> y = (-3 * HEIGHT) + rand()%((6*HEIGHT)-64);
        
        list_push( bad_presents , object , free );
    }


    if (bgmusic) {
        if (!(sample_data[0] = al_load_sample(bgmusic))) {
            n_log(LOG_ERR, "Could not load %s", bgmusic);
            exit(1);
        }
        al_play_sample(sample_data[0], 1, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);
    }

    __n_assert((santaSledgebmp = al_load_bitmap("DATA/Gfxs/santaSledge.png")), n_log(LOG_ERR, "load bitmap DATA/Gfxs/santaSledge.png returned null"); exit(1););

    init_vehicle(&santaSledge, WIDTH / 2, HEIGHT / 2);

    init_particle_system(&particle_system, INT_MAX, 0, 0, 0, 100);

    thread_pool = new_thread_pool(get_nb_cpu_cores(), 0);

    n_log(LOG_INFO, "Starting %d threads", get_nb_cpu_cores());

    al_flush_event_queue(event_queue);
    al_set_mouse_xy(display, WIDTH / 3, HEIGHT / 2);

    int w = al_get_display_width(display);
    int h = al_get_display_height(display);

    bitmap = al_create_bitmap(WIDTH, HEIGHT);

    DONE = 0;
    do {
        // consume events
        do {
            ALLEGRO_EVENT ev;

            al_wait_for_event(event_queue, &ev);

            if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_UP:
                        key[KEY_UP] = 1;
                        break;
                    case ALLEGRO_KEY_DOWN:
                        key[KEY_DOWN] = 1;
                        break;
                    case ALLEGRO_KEY_LEFT:
                        key[KEY_LEFT] = 1;
                        break;
                    case ALLEGRO_KEY_RIGHT:
                        key[KEY_RIGHT] = 1;
                        break;
                    case ALLEGRO_KEY_ESCAPE:
                        key[KEY_ESC] = 1;
                        break;
                    case ALLEGRO_KEY_SPACE:
                        key[KEY_SPACE] = 1;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                    case ALLEGRO_KEY_RSHIFT:
                        key[KEY_SHIFT] = 1;
                        break;
                    case ALLEGRO_KEY_PAD_MINUS:
                        key[KEY_PAD_MINUS] = 1;
                        break;
                    case ALLEGRO_KEY_PAD_PLUS:
                        key[KEY_PAD_PLUS] = 1;
                        break;
                    case ALLEGRO_KEY_PAD_ENTER:
                        key[KEY_PAD_ENTER] = 1;
                        break;
                    case ALLEGRO_KEY_M:
                        key[KEY_M] = 1;
                        break;
                    case ALLEGRO_KEY_W:
                        key[KEY_W] = 1;
                        break;
                    case ALLEGRO_KEY_LCTRL:
                    case ALLEGRO_KEY_RCTRL:
                        key[KEY_CTRL] = 1;
                        break;
                    case ALLEGRO_KEY_F1:
                        key[KEY_F1] = 1;
                        break;
                    case ALLEGRO_KEY_F2:
                        key[KEY_F2] = 1;
                        break;
                    case ALLEGRO_KEY_F3:
                        key[KEY_F3] = 1;
                        break;
                    case ALLEGRO_KEY_F4:
                        key[KEY_F4] = 1;
                        break;
                    case ALLEGRO_KEY_F5:
                        key[KEY_F5] = 1;
                        break;
                    case ALLEGRO_KEY_F6:
                        key[KEY_F6] = 1;
                        break;
                    default:
                        break;
                }
            } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_UP:
                        key[KEY_UP] = 0;
                        break;
                    case ALLEGRO_KEY_DOWN:
                        key[KEY_DOWN] = 0;
                        break;
                    case ALLEGRO_KEY_LEFT:
                        key[KEY_LEFT] = 0;
                        break;
                    case ALLEGRO_KEY_RIGHT:
                        key[KEY_RIGHT] = 0;
                        break;
                    case ALLEGRO_KEY_ESCAPE:
                        key[KEY_ESC] = 0;
                        break;
                    case ALLEGRO_KEY_SPACE:
                        key[KEY_SPACE] = 0;
                        break;
                    case ALLEGRO_KEY_LSHIFT:
                    case ALLEGRO_KEY_RSHIFT:
                        key[KEY_SHIFT] = 0;
                        break;
                    case ALLEGRO_KEY_PAD_MINUS:
                        key[KEY_PAD_MINUS] = 0;
                        break;
                    case ALLEGRO_KEY_PAD_PLUS:
                        key[KEY_PAD_PLUS] = 0;
                        break;
                    case ALLEGRO_KEY_PAD_ENTER:
                        key[KEY_PAD_ENTER] = 0;
                        break;
                    case ALLEGRO_KEY_M:
                        key[KEY_M] = 0;
                        break;
                    case ALLEGRO_KEY_W:
                        key[KEY_W] = 0;
                        break;
                    case ALLEGRO_KEY_LCTRL:
                    case ALLEGRO_KEY_RCTRL:
                        key[KEY_CTRL] = 0;
                        break;
                    case ALLEGRO_KEY_F1:
                        key[KEY_F1] = 0;
                        break;
                    case ALLEGRO_KEY_F2:
                        key[KEY_F2] = 0;
                        break;
                    case ALLEGRO_KEY_F3:
                        key[KEY_F3] = 0;
                        break;
                    case ALLEGRO_KEY_F4:
                        key[KEY_F4] = 0;
                        break;
                    case ALLEGRO_KEY_F5:
                        key[KEY_F5] = 0;
                        break;
                    case ALLEGRO_KEY_F6:
                        key[KEY_F6] = 0;
                        break;

                    default:
                        break;
                }
            } else if (ev.type == ALLEGRO_EVENT_TIMER) {
                if (al_get_timer_event_source(fps_timer) == ev.any.source) {
                    do_draw = 1;
                } else if (al_get_timer_event_source(logic_timer) == ev.any.source) {
                    do_logic = 1;
                }
            } else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
                mx = ev.mouse.x;
                my = ev.mouse.y;
            } else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (ev.mouse.button == 1)
                    mouse_b1 = 1;
                if (ev.mouse.button == 2)
                    mouse_b2 = 1;
            } else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
                if (ev.mouse.button == 1)
                    mouse_b1 = 0;
                if (ev.mouse.button == 2)
                    mouse_b2 = 0;
            }

            /* Processing inputs */
            mouse_button = -1;
            if (mouse_b1 == 1)
                mouse_button = 1;
            if (mouse_b2 == 1)
                mouse_button = 2;
            else if (ev.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN ||
                    ev.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
                al_clear_keyboard_state(display);
                al_flush_event_queue(event_queue);
            }
        } while (!al_is_event_queue_empty(event_queue));

        if (do_logic == 1) {
            start_HiTimer(&logic_chrono);
            // Processing inputs
            // get_keyboard( chat_line , ev );
            if (key[KEY_F1]) {
                set_vehicle_properties(&santaSledge, 2.0, 45.0, 150.0, 0.5);
            }
            if (key[KEY_F2]) {
                set_vehicle_properties(&santaSledge, 4.0, 60.0, 150.0, 1.0);
            }
            if (key[KEY_F3]) {
                set_vehicle_properties(&santaSledge, 6.0, 90.0, 150.0, 2.0);
            }
            if (key[KEY_F4]) {
            }
            if (key[KEY_F5]) {
            }
            if (key[KEY_F6]) {
            }
            set_handbrake(&santaSledge, 0.0);
            if (key[KEY_LEFT] && santaSledge.speed > 30.0) {
                steer_vehicle(&santaSledge, -2.0);
                if (key[KEY_SPACE]) {
                    set_handbrake(&santaSledge, 1.0);
                }
            } else if (key[KEY_RIGHT] && santaSledge.speed > 30.0) {
                steer_vehicle(&santaSledge, 2.0);
                if (key[KEY_SPACE]) {
                    set_handbrake(&santaSledge, -1.0);
                }
            } else if (key[KEY_SPACE]) {
                brake_vehicle(&santaSledge, 1.0 / logicFPS);
            }

            if (key[KEY_UP]) {
                if (santaSledge.handbrake == 0)
                    accelerate_vehicle(&santaSledge, 1.0 / logicFPS);
            } else {
                if (santaSledge.speed <= 30)
                    santaSledge.speed = 0;
            }
            if (key[KEY_DOWN]) {
                brake_vehicle(&santaSledge, 1.0 / logicFPS);
            }

            if (key[KEY_PAD_PLUS]) {
            }
            if (key[KEY_PAD_MINUS]) {
            }
            if (mouse_button) {
                // n_log( LOG_DEBUG , "mouse button: %d" , mouse_button );
            }

            /*
               static int old_mx = -1, old_my = -1;
               double mx_delta = 0.0, my_delta = 0.0;
               if (old_mx != mx || old_my != my) {
               if (old_mx != -1 && old_my != -1) {
               mx_delta = (old_mx - mx);
               my_delta = (old_my - my);
               }
               old_mx = mx;
               old_my = my;
               }

               santaSledge.x -= mx_delta ;
               santaSledge.y -= my_delta ;
               */
            double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            calculate_perpendicular_points(santaSledge.x, santaSledge.y, santaSledge.direction, 20.0, &x1, &y1, &x2, &y2);

            PHYSICS tmp_part;
            memset(&tmp_part, 0, sizeof(PHYSICS));
            tmp_part.type = 1;
            VECTOR3D_SET(tmp_part.speed, 0.0, 0.0, 0.0);

            if (santaSledge.handbrake) {
                // red
                VECTOR3D_SET(tmp_part.position, x1 + 2 - rand() % 4, y1 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(55 + rand() % 200, 0, 0), tmp_part);
                // green
                VECTOR3D_SET(tmp_part.position, x1 + 2 - rand() % 4, y1 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(0, 55 + rand() % 200, 0), tmp_part);
                // blue
                VECTOR3D_SET(tmp_part.position, x1 + 2 - rand() % 4, y1 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(0, 0, 55 + rand() % 200), tmp_part);
                // red
                VECTOR3D_SET(tmp_part.position, x2 + 2 - rand() % 4, y2 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(55 + rand() % 200, 0, 0), tmp_part);
                // green
                VECTOR3D_SET(tmp_part.position, x2 + 2 - rand() % 4, y2 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(0, 55 + rand() % 200, 0), tmp_part);
                // blue
                VECTOR3D_SET(tmp_part.position, x2 + 2 - rand() % 4, y2 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(0, 0, 55 + rand() % 200), tmp_part);
            }
            else if( santaSledge.speed > 0 )
            {
                int grey_value=50+rand()%100;
                // grey
                VECTOR3D_SET(tmp_part.position, x1 + 2 - rand() % 4, y1 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(grey_value,grey_value,grey_value), tmp_part);
                // grey
                VECTOR3D_SET(tmp_part.position, x1 + 2 - rand() % 4, y1 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(grey_value,grey_value,grey_value), tmp_part);
                // grey
                VECTOR3D_SET(tmp_part.position, x2 + 2 - rand() % 4, y2 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(grey_value,grey_value,grey_value), tmp_part);
                // grey
                VECTOR3D_SET(tmp_part.position, x2 + 2 - rand() % 4, y2 + 2 - rand() % 4, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, 1+rand()%3, al_map_rgb(grey_value,grey_value,grey_value), tmp_part);
            }
            // particles on good things
            list_foreach( node , good_presents )
            {
                gift_dash_object *object = node -> ptr ;
                VECTOR3D_SET(tmp_part.position, object->x+object->w/2, object->y+object->h/2, 0.0);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(55 + rand() % 200,0,0,50+rand()%200), tmp_part);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(0,55 + rand() % 200,0,50+rand()%200), tmp_part);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(0,0,55 + rand() % 200,50+rand()%200), tmp_part);
            }

            // particles on bad things
            list_foreach( node , bad_presents )
            {
                gift_dash_object *object = node -> ptr ;
                VECTOR3D_SET(tmp_part.position, object->x+object->w/2, object->y+object->h/2, 0.0);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(0 ,0,0 ,50+rand()%200), tmp_part);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(0 ,0,0 ,50+rand()%200), tmp_part);
                VECTOR3D_SET(tmp_part.speed, (-5.0+rand()%11)/50.0,(-5.0+rand()%11)/50.0, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 700000, 1+rand()%7, al_map_rgba(0 ,0,0,50+rand()%200 ), tmp_part);
            }


            manage_particle_ex(particle_system, 1000000000 / logicFPS);

            update_vehicle(&santaSledge, 1.0 / logicFPS);
            // print_vehicle(&santaSledge);

            if (santaSledge.x < -4 * WIDTH)
                santaSledge.x = 4 * WIDTH;
            if (santaSledge.x > 4 * WIDTH)
                santaSledge.x = -4 * WIDTH;
            if (santaSledge.y < -4 * HEIGHT)
                santaSledge.y = 4 * HEIGHT;
            if (santaSledge.y > 4 * HEIGHT)
                santaSledge.y = -4 * HEIGHT;

            tx = santaSledge.x - WIDTH / 2;
            ty = santaSledge.y - HEIGHT / 2;

            // Check collision
            collision = check_collision(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x, santaSledge.y, DEG_TO_RAD(santaSledge.direction), testRect);

            logic_duration = (logic_duration + get_usec(&logic_chrono)) / 2;
            do_logic = 0;
        }
        if (do_draw == 1) {
            start_HiTimer(&drawing_chrono);

            if (backbuffer)
                scrbuf = al_get_backbuffer(display);
            else
                scrbuf = bitmap;

            al_set_target_bitmap(scrbuf);

            if (!backbuffer)
                al_lock_bitmap(scrbuf, al_get_bitmap_format(scrbuf),
                        ALLEGRO_LOCK_READWRITE);

            // clear screen
            al_clear_to_color(al_map_rgb(175, 175, 175));

            // draw particles
            draw_particle(particle_system, tx, ty, w, h, 50);

            // show car DEBUG
            if (get_log_level() == LOG_DEBUG) {
                // draw sledge and collision point
                debug_draw_rotated_bitmap(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x - tx, santaSledge.y - ty, DEG_TO_RAD(santaSledge.direction));
                // draw mouse
                al_draw_circle(mx, my, 16, al_map_rgb(255, 0, 0), 2.0);
                // draw sledge position
                al_draw_circle(santaSledge.x, santaSledge.y, 20, al_map_rgb(255, 255, 0), 3.0);
                double car_angle = (santaSledge.direction * M_PI) / 180.0;
                double length = 10;
                double end_x = santaSledge.x + length * cos(car_angle);
                double end_y = santaSledge.y + length * sin(car_angle);
                // draw direction
                al_draw_line(santaSledge.x, santaSledge.y, end_x, end_y, al_map_rgb(255, 255, 0), 2.0);
                // print  speed
                static N_STR* textout = NULL;
                nstrprintf(textout, "Speed: %f", santaSledge.speed);
                al_draw_text(font, al_map_rgb(0, 0, 255), WIDTH, 10, ALLEGRO_ALIGN_RIGHT, _nstr(textout));
            } else {
                // draw santaSledge
                // al_draw_rotated_bitmap(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x, santaSledge.y, DEG_TO_RAD(santaSledge.direction), 0);
                al_draw_rotated_bitmap(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, WIDTH / 2, HEIGHT / 2, DEG_TO_RAD(santaSledge.direction), 0);
            }

            double testX = testRect.x + testRect.width / 2;
            double testY = testRect.y + testRect.height / 2;
            double dx = testX - santaSledge.x;
            double dy = testY - santaSledge.y;
            double testDist = sqrt(dx * dx + dy * dy);

            // Calculate the arrowhead position
            double arrowLength = 10.0;          // Length of the arrowhead
            double arrowAngle = atan2(dy, dx);  // Angle of the vector

            // Calculate the two arrowhead lines
            double arrowX1 = (WIDTH / 2 + (50 * dx) / testDist) - arrowLength * cos(arrowAngle - M_PI / 6);
            double arrowY1 = (HEIGHT / 2 + (50 * dy) / testDist) - arrowLength * sin(arrowAngle - M_PI / 6);
            double arrowX2 = (WIDTH / 2 + (50 * dx) / testDist) - arrowLength * cos(arrowAngle + M_PI / 6);
            double arrowY2 = (HEIGHT / 2 + (50 * dy) / testDist) - arrowLength * sin(arrowAngle + M_PI / 6);

            // Draw the arrowhead lines
            al_draw_line(WIDTH / 2 + (50 * dx) / testDist, HEIGHT / 2 + (50 * dy) / testDist, arrowX1, arrowY1, al_map_rgb(255, 0, 0), 2.0);
            al_draw_line(WIDTH / 2 + (50 * dx) / testDist, HEIGHT / 2 + (50 * dy) / testDist, arrowX2, arrowY2, al_map_rgb(255, 0, 0), 2.0);

            al_draw_line(WIDTH / 2, HEIGHT / 2, WIDTH / 2 + (50 * dx) / testDist, HEIGHT / 2 + (50 * dy) / testDist, al_map_rgb(255, 0, 0), 2.0);

            // Draw Rectangle
            al_draw_rectangle(testRect.x - tx, testRect.y - ty, testRect.x + testRect.width - tx, testRect.y + testRect.height - ty,
                    collision ? al_map_rgb(255, 0, 0) : al_map_rgb(0, 255, 0), 2);

            // draw goods
            list_foreach( node , good_presents )
            {
                gift_dash_object *object = node -> ptr ;
                al_draw_bitmap(christmasPresents[object->id],object->x-tx,object->y-ty, 0);
            }

            // draw bad
            list_foreach( node , bad_presents )
            {
                gift_dash_object *object = node -> ptr ;
                al_draw_bitmap(bogeymanPresents[object->id],object->x-tx,object->y-ty, 0);
            }

            if (!backbuffer) {
                al_unlock_bitmap(scrbuf);
                al_set_target_bitmap(al_get_backbuffer(display));
                al_draw_bitmap(scrbuf, w / 2 - al_get_bitmap_width(scrbuf) / 2,
                        h / 2 - al_get_bitmap_height(scrbuf) / 2, 0);
            }

            drawing_duration = (drawing_duration + get_usec(&drawing_chrono)) / 2;
            al_flip_display();
            do_draw = 0;
        }

    } while (!key[KEY_ESC] && !DONE);

    al_uninstall_system();

    return 0;
}
