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

// 2D_VEHICLE structure
typedef struct 2D_VEHICLE {
    float x, y;             // Position in units
    float speed;            // Forward speed in units per second
    float direction;        // Direction in degrees (0 = right, 90 = up)
    float drift_angle;      // Drift offset in degrees
    float lateral_velocity; // Lateral velocity in units per second
    float mass;             // Mass in kilograms
} 2D_VEHICLE;

// Initialize a 2D_VEHICLE
void initCar(2D_VEHICLE *vehicle, float startX, float startY, float mass);
// Accelerate the 2D_VEHICLE
void accelerate(2D_VEHICLE *vehicle);
// Brake the 2D_VEHICLE
void brake(2D_VEHICLE *vehicle);
// Steer the 2D_VEHICLE (adjust direction and induce lateral velocity)
void steer(2D_VEHICLE *vehicle, float angle);
// Apply drifting effect
void drift(2D_VEHICLE *vehicle, float driftAngle);
// Update 2D_VEHICLE position and physics
void updateCar(2D_VEHICLE *vehicle);
// Display 2D_VEHICLE status
void printCarStatus(const 2D_VEHICLE *vehicle);
