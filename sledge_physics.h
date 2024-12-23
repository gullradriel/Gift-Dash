/**\file states_management.h
 *  level file for hacks
 *\author Castagnier Mickaël aka Gull Ra Driel
 *\version 1.0
 *\date 29/12/2021
 */

#ifndef SLEDGE_HEADER_FOR_HACKS
#define SLEDGE_HEADER_FOR_HACKS

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

// Constants
//
// Maximum speed in pixels per second
#define MAX_SPEED 2000.0f
// Acceleration rate in pixels per second^2
#define ACCELERATION 400.0f
// Brake deceleration rate in pixels per second^2
#define BRAKE_FORCE 800.0f
// Steering angle in degrees per input
#define STEERING_ANGLE 5.0f
// Pi constant for trigonometric calculations
#define PI 3.14159265359f

// Convert degrees to radians
#define DEG_TO_RAD(angleDegrees) ((angleDegrees)*M_PI / 180.0)

// VEHICLE structure
typedef struct VEHICLE {
    double x, y;              // Position in units
    double speed;             // Forward speed in units per second
    double direction;         // Direction in degrees (0 = right, 90 = up)
    double angular_velocity;  // Rotation speed in degrees per second
    double slip_angle;        // Angle of lateral slip in degrees
    double handbrake;         // Handbrake state (0 = off, 1 = fully on)

    // vehicle properties
    double slip_factor;                  // make the vehicle slide more or less sideways (0.0 to 5.0)
    double angular_velocity_multiplier;  // amount of rotational spin (100.0 or more)
    double drag_multiplier;              // friction reduction (1.5 or more)
    double slip_angle_limits;            // clamp the maximum slip angle (default: 45.0)

} VEHICLE;

// calculate points perpandicular to a (x,y) source point and a direction
void calculate_perpendicular_points(double x, double y, double direction, double distance, double* x1, double* y1, double* x2, double* y2);
// Initialize a VEHICLE
void init_vehicle(VEHICLE* vehicle, double startX, double startY);
// Set properties
void set_vehicle_properties(VEHICLE* vehicle, double slip_factor, double slip_angle_limits, double angular_velocity_multiplier, double drag_multiplier);
// Accelerate the VEHICLE
void accelerate_vehicle(VEHICLE* vehicle, double delta_time);
// Brake the VEHICLE
void brake_vehicle(VEHICLE* vehicle, double delta_time);
// Steer the VEHICLE
void steer_vehicle(VEHICLE* vehicle, double angle);
// Apply handbrake to the VEHICLE
void set_handbrake(VEHICLE* vehicle, double value);
// Update VEHICLE position and physics
void update_vehicle(VEHICLE* vehicle, double delta_time);
// Display VEHICLE status
void print_vehicle(const VEHICLE* vehicle);

// Rectangle structure
typedef struct {
    double x, y;  // Top-left corner
    double w, h;  // Dimensions
} CollisionRectangle;

// Vector structure for SAT
typedef struct {
    double x, y;
} Vector;

// Function prototypes
// void calculate_rotated_corners(double dx, double dy, double cx, double cy, double width, double height, double angle, Vector corners[4]);
// bool sat_collision(Vector rectA[4], Vector rectB[4]);
// Vector subtract_vectors(Vector a, Vector b);
// double dot_product(Vector a, Vector b);
// Vector perpendicular(Vector v);
// double min_projection(Vector axis, Vector corners[4]);
// double max_projection(Vector axis, Vector corners[4]);
bool check_collision(ALLEGRO_BITMAP* bitmap, double cx, double cy, double dx, double dy, double angle, CollisionRectangle rect);
void debug_draw_rotated_bitmap(ALLEGRO_BITMAP* bitmap, double cx, double cy, double dx, double dy, double angle);

#ifdef __cplusplus
}
#endif

#endif
