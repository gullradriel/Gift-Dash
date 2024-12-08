#include <stdio.h>
#include <math.h>

// Constants
#define MAX_SPEED 100.0f        // Maximum speed in units per second
#define ACCELERATION 10.0f      // Acceleration rate in units per second^2
#define BRAKE_FORCE 20.0f       // Brake deceleration rate in units per second^2
#define STEERING_ANGLE 5.0f     // Steering angle in degrees per input
#define DRIFT_FACTOR 0.1f       // Drift decay factor (0 = no decay, 1 = full decay)
#define SKID_FRICTION 0.9f      // Friction coefficient for skid (0 = slippery, 1 = high grip)
#define DELTA_TIME 0.1f         // Simulation time step in seconds
#define PI 3.14159265359f       // Pi constant for trigonometric calculations

// Car structure
typedef struct {
    float x, y;             // Position in units
    float speed;            // Forward speed in units per second
    float direction;        // Direction in degrees (0 = right, 90 = up)
    float drift_angle;      // Drift offset in degrees
    float lateral_velocity; // Lateral velocity in units per second
    float mass;             // Mass in kilograms
} Car;

// Initialize a car
void initCar(Car *car, float startX, float startY, float mass) {
    car->x = startX;        // Initial X position
    car->y = startY;        // Initial Y position
    car->speed = 0.0f;      // Starting at rest
    car->direction = 0.0f;  // Facing right (0 degrees)
    car->drift_angle = 0.0f;
    car->lateral_velocity = 0.0f;
    car->mass = mass;       // Assign car mass
}

// Accelerate the car
void accelerate(Car *car) {
    car->speed += ACCELERATION * DELTA_TIME;
    if (car->speed > MAX_SPEED) {
        car->speed = MAX_SPEED;
    }
}

// Brake the car
void brake(Car *car) {
    car->speed -= BRAKE_FORCE * DELTA_TIME;
    if (car->speed < 0) {
        car->speed = 0;
    }
}

// Steer the car (adjust direction and induce lateral velocity)
void steer(Car *car, float angle) {
    car->direction += angle;
    if (car->direction >= 360.0f) car->direction -= 360.0f;
    if (car->direction < 0.0f) car->direction += 360.0f;

    // Lateral force causes skid, proportional to speed and turn angle
    float lateral_force = car->speed * sinf(angle * PI / 180.0f);
    car->lateral_velocity += (lateral_force / car->mass); // F = ma, a = F/m
}

// Apply drifting effect
void drift(Car *car, float driftAngle) {
    car->drift_angle = driftAngle;
}

// Update car position and physics
void updateCar(Car *car) {
    // Convert direction (with drift) to radians
    float directionRad = (car->direction + car->drift_angle) * PI / 180.0f;

    // Update forward motion
    car->x += car->speed * cosf(directionRad) * DELTA_TIME;
    car->y += car->speed * sinf(directionRad) * DELTA_TIME;

    // Apply lateral velocity (skid/slideslip)
    car->x += car->lateral_velocity * cosf((car->direction + 90.0f) * PI / 180.0f) * DELTA_TIME;
    car->y += car->lateral_velocity * sinf((car->direction + 90.0f) * PI / 180.0f) * DELTA_TIME;

    // Decay lateral velocity (friction inversely proportional to mass)
    car->lateral_velocity *= powf(SKID_FRICTION, DELTA_TIME * (100.0f / car->mass));

    // Reduce drift effect over time
    car->drift_angle *= (1.0f - DRIFT_FACTOR);
}

// Display car status
void printCarStatus(const Car *car) {
    printf("Position: (%.2f, %.2f), Speed: %.2f units/s, Direction: %.2f°, Drift: %.2f°, Lateral Vel: %.2f units/s\n",
           car->x, car->y, car->speed, car->direction, car->drift_angle, car->lateral_velocity);
}

