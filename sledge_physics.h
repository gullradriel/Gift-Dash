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

// VEHICLE structure
typedef struct VEHICLE {
    float x, y;             // Position in units
    float speed;            // Forward speed in units per second
    float direction;        // Direction in degrees (0 = right, 90 = up)
    float drift_angle;      // Drift offset in degrees
    float lateral_velocity; // Lateral velocity in units per second
    float mass;             // Mass in kilograms
} VEHICLE;

// Initialize a VEHICLE
void initVehicle(VEHICLE *vehicle, float startX, float startY, float mass);
// Accelerate the VEHICLE
void accelerateVehicle(VEHICLE *vehicle);
// Brake the VEHICLE
void brakeVehicle(VEHICLE *vehicle);
// Steer the VEHICLE (adjust direction and induce lateral velocity)
void steerVehicle(VEHICLE *vehicle, float angle);
// Apply drifting effect
void driftVehicle(VEHICLE *vehicle, float driftAngle);
// Update VEHICLE position and physics
void updateVehicle(VEHICLE *vehicle);
// Display VEHICLE status
void printVehicle(const VEHICLE *vehicle);
