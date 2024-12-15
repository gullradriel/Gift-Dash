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

ALLEGRO_DISPLAY* display = NULL;

/******************************************************************************
 *                           VARIOUS DECLARATIONS                             *
 ******************************************************************************/

int DONE = 0,            /* Flag to check if we are always running */
    getoptret = 0,       /* launch parameter check */
    log_level = LOG_ERR; /* default LOG_LEVEL */

double drawFPS = 60.0;
double logicFPS = 240.0;

ALLEGRO_TIMER* fps_timer = NULL;
ALLEGRO_TIMER* logic_timer = NULL;
N_TIME logic_chrono;
N_TIME drawing_chrono;

size_t WIDTH = 1280, HEIGHT = 800;
bool fullscreen = 0;
char* bgmusic = NULL;

THREAD_POOL* thread_pool = NULL;

#define DEG_TO_RAD(angleDegrees) ((angleDegrees) * M_PI / 180.0)

void calculate_perpendicular_points(double x, double y, double direction, double distance, 
                                    double *x1, double *y1, double *x2, double *y2) {
    // Convert direction to radians
    double angleRad = DEG_TO_RAD(direction);

    // Calculate the perpendicular angle (relative to direction)
    double perpAngle = angleRad + M_PI / 2; // Perpendicular at +90 degrees

    // Calculate the offsets
    double xOffset = distance * cos(perpAngle);
    double yOffset = distance * sin(perpAngle);

    // First perpendicular point
    *x1 = x + xOffset;
    *y1 = y + yOffset;

    // Second perpendicular point (opposite direction)
    *x2 = x - xOffset;
    *y2 = y - yOffset;
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

    ALLEGRO_SAMPLE* sample_data[MAX_SAMPLE_DATA] = {NULL};
    memset(sample_data, 0, sizeof(sample_data));

    ALLEGRO_EVENT_QUEUE* event_queue = NULL;

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

    // not working under linux ask why
    // al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP|ALLEGRO_NO_PRESERVE_TEXTURE
    // );

    display = al_create_display(WIDTH, HEIGHT);
    if (!display) {
        n_abort("Unable to create display\n");
    }

    al_set_window_title(display, "GiftDash");

    ALLEGRO_FONT* font = al_load_font("DATA/2Dumb.ttf", 18, 0);

    DONE = 0;
    fps_timer = al_create_timer(1.0 / drawFPS);
    logic_timer = al_create_timer(1.0 / logicFPS);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_start_timer(fps_timer);
    al_start_timer(logic_timer);
    al_register_event_source(event_queue, al_get_timer_event_source(fps_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));

    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    bool backbuffer = 1;
    ALLEGRO_BITMAP* scrbuf = NULL;
    ALLEGRO_BITMAP* bitmap = al_create_bitmap(WIDTH, HEIGHT);

    al_hide_mouse_cursor(display);

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
    int key[19] = {false, false, false, false, false, false, false, false, false,
                   false, false, false, false, false, false, false, false, false};

    if (bgmusic) {
        if (!(sample_data[0] = al_load_sample(bgmusic))) {
            n_log(LOG_ERR, "Could not load %s", bgmusic);
            exit(1);
        }
        al_play_sample(sample_data[0], 1, 0, 1, ALLEGRO_PLAYMODE_LOOP, NULL);
    }
    ALLEGRO_BITMAP *santaSledgebmp = NULL ;
	__n_assert( ( santaSledgebmp = al_load_bitmap( "DATA/Gfxs/santaSledge.png"  ) )     , n_log( LOG_ERR , "load bitmap DATA/Gfxs/santaSledge.png returned null" ); exit( 1 ); );

    VEHICLE santaSledge;
    init_vehicle(&santaSledge, WIDTH / 2, HEIGHT / 2);

    PARTICLE_SYSTEM* particle_system=NULL;
    init_particle_system(&particle_system, INT_MAX, 0, 0, 0, 100);

    thread_pool = new_thread_pool(get_nb_cpu_cores(), 0);

    n_log(LOG_INFO, "Starting %d threads", get_nb_cpu_cores());

    bool do_draw = 1, do_logic = 1;
    int mx = WIDTH / 3, my = HEIGHT / 2, mouse_button = 0, mouse_b1 = 0,
        mouse_b2 = 0;

    al_flush_event_queue(event_queue);
    al_set_mouse_xy(display, WIDTH / 3, HEIGHT / 2);

    int w = al_get_display_width(display);
    int h = al_get_display_height(display);

    bitmap = al_create_bitmap(WIDTH, HEIGHT);

    // Test rectangle
    Rectangle testRect = {300, 250, 100, 100};

    size_t logic_duration = 0;
    size_t drawing_duration = 0;
    bool collision = false ;
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

            if (santaSledge.handbrake) {
                
                double x1=0,y1=0,x2=0,y2=0;
                calculate_perpendicular_points( santaSledge.x, santaSledge.y, santaSledge.direction, 20.0, &x1, &y1, &x2, &y2);
                
                PHYSICS tmp_part;
                memset(&tmp_part, 0, sizeof(PHYSICS));
                tmp_part.sz = rand() % 10;
                tmp_part.type = 1;
                VECTOR3D_SET(tmp_part.speed, 0.0, 0.0, 0.0);
                VECTOR3D_SET(tmp_part.position, x1, y1, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, -1, al_map_rgba(rand()%250, rand()%250, rand()%250, rand() % 100), tmp_part);
                VECTOR3D_SET(tmp_part.position, x2, y2, 0.0);
                add_particle(particle_system, -1, PIXEL_PART, 60000000, -1, al_map_rgba(rand()%250, rand()%250, rand()%250, rand() % 100), tmp_part);
            }

            manage_particle_ex(particle_system,1000000000/logicFPS);

            update_vehicle(&santaSledge, 1.0 / logicFPS);
            //print_vehicle(&santaSledge);

            if (santaSledge.x < 0)
                santaSledge.x = WIDTH;
            if (santaSledge.x > WIDTH)
                santaSledge.x = 0;
            if (santaSledge.y < 0)
                santaSledge.y = HEIGHT;
            if (santaSledge.y > HEIGHT)
                santaSledge.y = 0;

            // Check collision
            //collision = check_bitmap_vs_rectangle(santaSledge.x, santaSledge.y, al_get_bitmap_width(santaSledgebmp),al_get_bitmap_height(santaSledgebmp), DEG_TO_RAD(santaSledge.direction), testRect);

            collision = check_collision( santaSledgebmp,0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x, santaSledge.y, DEG_TO_RAD(santaSledge.direction), testRect);

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
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // draw particles 
            draw_particle(particle_system, 0, 0, w, h, 50);

            //show car DEBUG
            if( get_log_level() == LOG_DEBUG )
            {
                //draw sledge and collision point
                debug_draw_rotated_bitmap(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x, santaSledge.y, DEG_TO_RAD(santaSledge.direction));
                // draw mouse
                al_draw_circle(mx, my, 16, al_map_rgb(255, 0, 0), 2.0);
                // draw sledge position
                al_draw_circle(santaSledge.x, santaSledge.y, 20, al_map_rgb(255, 255, 0), 3.0);
                double car_angle = (santaSledge.direction * M_PI) / 180.0;
                double length = 10;
                double end_x = santaSledge.x + length * cos(car_angle);
                double end_y = santaSledge.y + length * sin(car_angle);
                //draw direction
                al_draw_line(santaSledge.x, santaSledge.y, end_x, end_y, al_map_rgb(255, 255, 0), 2.0);
                // print  speed
                static N_STR* textout = NULL;
                nstrprintf(textout, "Speed: %f", santaSledge.speed);
                al_draw_text(font, al_map_rgb(0, 0, 255), WIDTH, 10, ALLEGRO_ALIGN_RIGHT, _nstr(textout));
            }
            else
            {
                // draw santaSledge
                al_draw_rotated_bitmap(santaSledgebmp, 0, al_get_bitmap_height(santaSledgebmp) / 2.0, santaSledge.x, santaSledge.y, DEG_TO_RAD(santaSledge.direction), 0);
            }

            // Draw result
            al_draw_rectangle(testRect.x, testRect.y, testRect.x + testRect.width, testRect.y + testRect.height,
                      collision ? al_map_rgb(255, 0, 0) : al_map_rgb(0, 255, 0), 2);

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
