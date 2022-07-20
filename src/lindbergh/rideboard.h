#include <stdio.h>

/* Enum to define the colour of the billboard lamp */
typedef enum
{
	LAMP_OFF = 0,
	LAMP_GREEN,
	LAMP_BLUE,
	LAMP_RED
} LampState;

/* Enums to represent the different states of the MotionSelectSwitch */
#define MOTION_OFF 0
#define MOTION_MILD 1
#define MOTION_NORMAL 2

/* Struct to hold the internal state of the ride */
typedef struct
{
	/* COMMANDS */
	int Command;
	int CommandReply;
	int SeatCommand;
	int SeatCommandReply;
	/* OUTPUTS */
	int PlayerOneBlowFront;
	int PlayerOneBlowBack;
	int PlayerTwoBlowFront;
	int PlayerTwoBlowBack;
	int PlayerOneGunReaction;
	int PlayerTwoGunReaction;
	int ResetLamp;
	int ErrorLamp;
	int SafetyLamp;
	int GameStopLamp;
	int FloorLamp;
	int SpotLamp;
	LampState BillboardLamp;
	/* INPUTS */
	int InitButton;
	int ResetButton;
	int MotionSelectSwitch; // 0 Motion Off, 1 Motion Gentle, 2 Motion Normal
	int TowerGameStopButton;
	int RideGameStopButton;
	int RearFootSensor;
	int LeftFootSensor;
	int FrontFootSensor;
	int RightFootSensor;
	int RightDoorSensor;
	int LeftDoorSensor;
	int ArmrestSensor;
	int PlayerOneSeatbeltSensor;
	int PlayerTwoSeatbeltSensor;
	int RearPositionSensor;
	int FrontPositionSensor;
	int CCWLimitSensor;
	int CWLimitSensor;
	int MotorPower;
} RideState;

int initRideboard();

ssize_t rideboardRead(int fd, void *buf, size_t count);
ssize_t rideboardWrite(int fd, const void *buf, size_t count);
