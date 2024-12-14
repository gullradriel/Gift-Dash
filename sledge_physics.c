#include "sledge_physics.h"
#include <math.h>
#include <stdio.h>
#include "nilorea/n_log.h"

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
