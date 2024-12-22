#include "sledge_physics.h"
#include <math.h>
#include <stdio.h>
#include "nilorea/n_log.h"

void calculate_perpendicular_points(double x, double y, double direction, double distance, double* x1, double* y1, double* x2, double* y2) {
    // Convert direction to radians
    double angleRad = DEG_TO_RAD(direction);

    // Calculate the perpendicular angle (relative to direction)
    double perpAngle = angleRad + M_PI / 2;  // Perpendicular at +90 degrees

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

// Initialize a vehicle
void init_vehicle(VEHICLE* vehicle, double startX, double startY) {
    vehicle->x = startX;        // Initial X position
    vehicle->y = startY;        // Initial Y position
    vehicle->speed = 0.0f;      // Starting at rest
    vehicle->direction = 0.0f;  // Facing right (0 degrees)

    // defaults properties
    vehicle->slip_factor = 2.0;
    vehicle->slip_angle_limits = 45.0;
    vehicle->angular_velocity_multiplier = 150.0;
    vehicle->drag_multiplier = 0.2;
}

void set_vehicle_properties(VEHICLE* vehicle, double slip_factor, double slip_angle_limits, double angular_velocity_multiplier, double drag_multiplier) {
    vehicle->slip_factor = slip_factor;
    vehicle->slip_angle_limits = slip_angle_limits;
    vehicle->angular_velocity_multiplier = angular_velocity_multiplier;
    vehicle->drag_multiplier = drag_multiplier;
}

// Accelerate the vehicle
void accelerate_vehicle(VEHICLE* vehicle, double delta_time) {
    vehicle->speed += ACCELERATION * delta_time;
    if (vehicle->speed > MAX_SPEED) {
        vehicle->speed = MAX_SPEED;
    }
}

// Brake the vehicle
void brake_vehicle(VEHICLE* vehicle, double delta_time) {
    vehicle->speed -= BRAKE_FORCE * delta_time;
    if (vehicle->speed < 0) {
        vehicle->speed = 0;
    }
}

// Steer the vehicle
void steer_vehicle(VEHICLE* vehicle, double angle) {
    vehicle->direction += angle;
    if (vehicle->direction >= 360.0f)
        vehicle->direction -= 360.0f;
    if (vehicle->direction < 0.0f)
        vehicle->direction += 360.0f;
}

// amplify handbrake vehicle reaction
void exaggerate_slip(VEHICLE* vehicle, double delta_time) {
    if (fabs(vehicle->handbrake) > 0) {
        // Increase slip angle significantly with the handbrake
        vehicle->slip_angle += vehicle->angular_velocity * vehicle->slip_factor * delta_time;
    }

    // Clamp slip angle for stability
    if (vehicle->slip_angle > vehicle->slip_angle_limits) vehicle->slip_angle = vehicle->slip_angle_limits;  // Limit to avoid extreme values
    if (vehicle->slip_angle < -vehicle->slip_angle_limits) vehicle->slip_angle = -vehicle->slip_angle_limits;
}

// Set handbrake for vehicle
void set_handbrake(VEHICLE* vehicle, double value) {
    vehicle->handbrake = value;
    // fmax(-0.0, fmin(1.0, value));  // Clamp between 0 and 1
}

// Decay angular velocity to avoid bad simulation behavior
void reset_angular_velocity(VEHICLE* vehicle, double decay_rate, double delta_time) {
    // Apply decay to angular velocity (simulate friction restoring stability)
    vehicle->angular_velocity *= (1.0 - decay_rate * delta_time);

    // If angular velocity is very small, reset to zero to prevent drift
    if (fabs(vehicle->angular_velocity) < 0.01) {
        vehicle->angular_velocity = 0.0;
    }
}

// Manage handbrake for vehicle
void handbrake_vehicle(VEHICLE* vehicle, double delta_time) {
    if (fabs(vehicle->handbrake) > 0) {
        // Simulate friction slowing the vehicle
        vehicle->speed *= (1.0 - fabs(vehicle->handbrake) * vehicle->drag_multiplier * delta_time);

        // Increase angular velocity to simulate easier spinning
        vehicle->angular_velocity += vehicle->handbrake * vehicle->angular_velocity_multiplier * delta_time;  // Tunable value
    } else {
        // Decay angular velocity when handbrake is released
        reset_angular_velocity(vehicle, 10.0, delta_time);
    }
}

// Update vehicle with handbrake and slip
void update_slip(VEHICLE* vehicle, double delta_time) {
    double lateral_friction = (fabs(vehicle->handbrake) > 0) ? 0.2 : 0.9;  // Less grip with handbrake
    vehicle->slip_angle = (1.0 - lateral_friction) * vehicle->angular_velocity;

    // Adjust the effective direction with slip
    double effective_direction = vehicle->direction + vehicle->slip_angle;
    vehicle->x += vehicle->speed * cos(effective_direction * M_PI / 180.0) * delta_time;
    vehicle->y += vehicle->speed * sin(effective_direction * M_PI / 180.0) * delta_time;
}

// Stabilize the vehicle if no handbrake and not turning too much
void stabilize_traction(VEHICLE* vehicle, double delta_time) {
    if (vehicle->handbrake == 0 && fabs(vehicle->slip_angle) < 5.0) {  // Threshold for near-straight movement
        reset_angular_velocity(vehicle, 5.0, delta_time);              // Rapid stabilization
    }
}

// Reset angular_velocity, like when hitting an obstacle
void hard_reset_angular_velocity(VEHICLE* vehicle) {
    vehicle->angular_velocity = 0.0;
}

// Update vehicle position and physics
void update_vehicle(VEHICLE* vehicle, double delta_time) {
    // Convert direction (with drift) to radians
    double directionRad = (vehicle->direction * PI) / 180.0f;

    // Update forward motion
    vehicle->x += vehicle->speed * cosf(directionRad) * delta_time;
    vehicle->y += vehicle->speed * sinf(directionRad) * delta_time;

    // Update direction with angular velocity
    vehicle->direction += vehicle->angular_velocity * delta_time;
    vehicle->direction = fmod(vehicle->direction, 360.0);  // Normalize to 0-360 degrees

    // apply handbrake
    handbrake_vehicle(vehicle, delta_time);

    // apply slide slip
    update_slip(vehicle, delta_time);

    // exagerate drift
    exaggerate_slip(vehicle, delta_time);

    // Stabilize angular velocity based on traction and decay rate
    stabilize_traction(vehicle, delta_time);

    // Ensure angular velocity does not grow uncontrollably
    reset_angular_velocity(vehicle, 1.0, delta_time);  // Default decay rate
}

// Display vehicle status
void print_vehicle(const VEHICLE* vehicle) {
    n_log(LOG_DEBUG, "Position: (%.2f, %.2f), Speed: %.2f units/s, Direction: %.2f°",
          vehicle->x, vehicle->y, vehicle->speed, vehicle->direction);
}

void calculate_rotated_corners(double dx, double dy, double cx, double cy, double width, double height, double angle, Vector corners[4]) {
    // Bitmap corners relative to its center of rotation (cx, cy)
    Vector points[4] = {
        {-cx, -cy},                 // Top-left relative to cx, cy
        {width - cx, -cy},          // Top-right relative to cx, cy
        {width - cx, height - cy},  // Bottom-right relative to cx, cy
        {-cx, height - cy}          // Bottom-left relative to cx, cy
    };

    // Rotate each corner around the center `(cx, cy)` and translate to `(dx, dy)`
    double cosA = cos(angle);
    double sinA = sin(angle);

    for (int i = 0; i < 4; i++) {
        corners[i].x = dx + (points[i].x * cosA - points[i].y * sinA);
        corners[i].y = dy + (points[i].x * sinA + points[i].y * cosA);
    }
}

// Subtract two vectors
Vector subtract_vectors(Vector a, Vector b) {
    Vector result = {a.x - b.x, a.y - b.y};
    return result;
}

// Dot product of two vectors
double dot_product(Vector a, Vector b) {
    return a.x * b.x + a.y * b.y;
}

// Get perpendicular vector
Vector perpendicular(Vector v) {
    Vector result = {-v.y, v.x};
    return result;
}

// Get the minimum projection of corners onto an axis
double min_projection(Vector axis, Vector corners[4]) {
    double min = dot_product(axis, corners[0]);
    for (int i = 1; i < 4; i++) {
        double projection = dot_product(axis, corners[i]);
        if (projection < min) min = projection;
    }
    return min;
}

// Get the maximum projection of corners onto an axis
double max_projection(Vector axis, Vector corners[4]) {
    double max = dot_product(axis, corners[0]);
    for (int i = 1; i < 4; i++) {
        double projection = dot_product(axis, corners[i]);
        if (projection > max) max = projection;
    }
    return max;
}

// project polygon on axis
void project_polygon(Vector poly[4], int count, Vector axis, double* min, double* max) {
    *min = *max = (poly[0].x * axis.x + poly[0].y * axis.y);
    for (int i = 1; i < count; i++) {
        double projection = (poly[i].x * axis.x + poly[i].y * axis.y);
        if (projection < *min) *min = projection;
        if (projection > *max) *max = projection;
    }
}

// check overlap on axis
bool overlap_on_axis(Vector poly1[4], Vector poly2[4], Vector axis) {
    // Project poly1 and poly2 onto the axis
    double min1, max1, min2, max2;
    project_polygon(poly1, 4, axis, &min1, &max1);
    project_polygon(poly2, 4, axis, &min2, &max2);

    // Check if projections overlap
    return !(max1 < min2 || max2 < min1);
}

// separated axis theorem collision
bool sat_collision(Vector poly1[4], Vector poly2[4]) {
    // Check axes of poly1
    for (int i = 0; i < 4; i++) {
        Vector edge = {poly1[(i + 1) % 4].x - poly1[i].x, poly1[(i + 1) % 4].y - poly1[i].y};
        Vector axis = {-edge.y, edge.x};  // Perpendicular axis

        if (!overlap_on_axis(poly1, poly2, axis)) {
            return false;
        }
    }

    // Check axes of poly2
    for (int i = 0; i < 4; i++) {
        Vector edge = {poly2[(i + 1) % 4].x - poly2[i].x, poly2[(i + 1) % 4].y - poly2[i].y};
        Vector axis = {-edge.y, edge.x};  // Perpendicular axis

        if (!overlap_on_axis(poly1, poly2, axis)) {
            return false;
        }
    }

    return true;  // Collision detected on all axes
}

// check collisions between rotated bitmap and a rect
bool check_collision(ALLEGRO_BITMAP* bitmap, double cx, double cy, double dx, double dy, double angle, Rectangle rect) {
    Vector bitmap_corners[4];
    double bitmap_width = al_get_bitmap_width(bitmap);
    double bitmap_height = al_get_bitmap_height(bitmap);

    // Calculate the rotated corners of the bitmap
    calculate_rotated_corners(dx, dy, cx, cy, bitmap_width, bitmap_height, angle, bitmap_corners);

    // Get the rectangle's corners
    Vector rect_corners[4] = {
        {rect.x, rect.y},                             // Top-left
        {rect.x + rect.width, rect.y},                // Top-right
        {rect.x + rect.width, rect.y + rect.height},  // Bottom-right
        {rect.x, rect.y + rect.height}                // Bottom-left
    };

    // Check for collision using SAT
    return sat_collision(bitmap_corners, rect_corners);
}

// draw debug collision box
void debug_draw_rotated_bitmap(ALLEGRO_BITMAP* bitmap, double cx, double cy, double dx, double dy, double angle) {
    Vector corners[4];
    double width = al_get_bitmap_width(bitmap);
    double height = al_get_bitmap_height(bitmap);

    calculate_rotated_corners(dx, dy, cx, cy, width, height, angle, corners);

    // Draw the rotated bitmap
    al_draw_rotated_bitmap(bitmap, cx, cy, dx, dy, angle, 0);

    // Draw collision bounds
    for (int i = 0; i < 4; i++) {
        Vector start = corners[i];
        Vector end = corners[(i + 1) % 4];
        al_draw_line(start.x, start.y, end.x, end.y, al_map_rgb(0, 255, 0), 2);
    }
}
