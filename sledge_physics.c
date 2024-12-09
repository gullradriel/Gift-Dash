#include <stdio.h>
#include <math.h>
#include "sledge_physics.h"

// Initialize a vehicle
void initVehicle(VEHICLE *vehicle, float startX, float startY, float mass) {
    vehicle->x = startX;        // Initial X position
    vehicle->y = startY;        // Initial Y position
    vehicle->speed = 0.0f;      // Starting at rest
    vehicle->direction = 0.0f;  // Facing right (0 degrees)
    vehicle->drift_angle = 0.0f;
    vehicle->lateral_velocity = 0.0f;
    vehicle->mass = mass;       // Assign vehicle mass
}

// Accelerate the vehicle
void accelerateVehicle(VEHICLE *vehicle) {
    vehicle->speed += ACCELERATION * DELTA_TIME;
    if (vehicle->speed > MAX_SPEED) {
        vehicle->speed = MAX_SPEED;
    }
}

// Brake the vehicle
void brakeVehicle(VEHICLE *vehicle) {
    vehicle->speed -= BRAKE_FORCE * DELTA_TIME;
    if (vehicle->speed < 0) {
        vehicle->speed = 0;
    }
}

// Steer the vehicle (adjust direction and induce lateral velocity)
void steerVehicle(VEHICLE *vehicle, float angle) {
    vehicle->direction += angle;
    if (vehicle->direction >= 360.0f) vehicle->direction -= 360.0f;
    if (vehicle->direction < 0.0f) vehicle->direction += 360.0f;

    // Lateral force causes skid, proportional to speed and turn angle
    float lateral_force = vehicle->speed * sinf(angle * PI / 180.0f);
    vehicle->lateral_velocity += (lateral_force / vehicle->mass); // F = ma, a = F/m
}

// Apply drifting effect
void driftVehicle(VEHICLE *vehicle, float driftAngle) {
    vehicle->drift_angle = driftAngle;
}

// Update vehicle position and physics
void updateVehicle(VEHICLE *vehicle) {
    // Convert direction (with drift) to radians
    float directionRad = (vehicle->direction + vehicle->drift_angle) * PI / 180.0f;

    // Update forward motion
    vehicle->x += vehicle->speed * cosf(directionRad) * DELTA_TIME;
    vehicle->y += vehicle->speed * sinf(directionRad) * DELTA_TIME;

    // Apply lateral velocity (skid/slideslip)
    vehicle->x += vehicle->lateral_velocity * cosf((vehicle->direction + 90.0f) * PI / 180.0f) * DELTA_TIME;
    vehicle->y += vehicle->lateral_velocity * sinf((vehicle->direction + 90.0f) * PI / 180.0f) * DELTA_TIME;

    // Decay lateral velocity (friction inversely proportional to mass)
    vehicle->lateral_velocity *= powf(SKID_FRICTION, DELTA_TIME * (100.0f / vehicle->mass));

    // Reduce drift effect over time
    vehicle->drift_angle *= (1.0f - DRIFT_FACTOR);
}

// Display vehicle status
void printVehicle(const VEHICLE *vehicle) {
    printf("Position: (%.2f, %.2f), Speed: %.2f units/s, Direction: %.2f°, Drift: %.2f°, Lateral Vel: %.2f units/s\n",
           vehicle->x, vehicle->y, vehicle->speed, vehicle->direction, vehicle->drift_angle, vehicle->lateral_velocity);
}

