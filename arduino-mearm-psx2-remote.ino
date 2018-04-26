#define HISTORY 1

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <PS2X_lib.h> 

#define PSX2_DATA 12      // D0 or PIN 1 on PSX controller
#define PSX2_CLOCK 11     // D1 or PIN 2 on PSX controller
#define PSX2_ATTENTION 10 // CS or PIN 6 on PSX controller
#define PSX2_COMMAND 9    // CLK or PIN 7 on PSX controller

#define PSX2_PRESSURES false
#define PSX2_RUMBLE false

PS2X ps2x;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// servo index

#define BASE_SERVO 0  // middle, turn left/right
#define LEFT_SERVO 1  // left, extend/constract arm
#define RIGHT_SERVO 2 // right forward/backward arm
#define CLAW_SERVO 3  // claw open/close

// PWM controller pins

uint8_t pwmPins[4] = { 0, 15, 1, 3 };

#define TOWERPRO_MG90S_FREQ 50 // from manual

#define CLAW_MIN_PULSE 300 // open
#define CLAW_MAX_PULSE 550 // closed

#define BASE_MIN_PULSE 120 // right dir
#define BASE_MAX_PULSE 440 // left dir

#define LEFT_MIN_PULSE 200 // arm contracted
#define LEFT_MAX_PULSE 350 // arm extended

#define RIGHT_MIN_PULSE 250 // arm contracted
#define RIGHT_MAX_PULSE 450 // arm extended


#define CLAW_OPEN_ANGLE 160 // not used
#define CLAW_CLOSE_ANGLE 0 // not used

#define BASE_LEFT_ANGLE 10 // not used
#define BASE_RIGHT_ANGLE 80 // not used


// actual min/max [135, 585] based on BASE servo
// try 150, 530 to make base centered
int servo_min_pulse[4] = { BASE_MIN_PULSE, LEFT_MIN_PULSE, RIGHT_MIN_PULSE, CLAW_MIN_PULSE };
int servo_max_pulse[4] = { BASE_MAX_PULSE, LEFT_MAX_PULSE, RIGHT_MAX_PULSE, CLAW_MAX_PULSE };

#define HISTORY 40

int8_t leftValues[HISTORY];
int8_t rightValues[HISTORY];
int8_t baseValues[HISTORY];
int8_t clawValues[HISTORY];

float currentPulseValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
float prevPulseValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

float valueScales[4] = { 5.0f, 10.0f, 10.0f, 10.0f };
float turbo = 1.0f;

uint8_t readIndex = 0;

void resetHistoryValues(int8_t values[HISTORY], int8_t resetValue)
{
    for (int i = 0; i < HISTORY; ++i)
    {
        values[i] = resetValue;
    }
}

void resetAllValues()
{
    Serial.println("Resetting values");
    Serial.println("-");
    currentPulseValues[LEFT_SERVO] = (servo_min_pulse[LEFT_SERVO] + servo_max_pulse[LEFT_SERVO]) / 2.0f;
    currentPulseValues[RIGHT_SERVO] = (servo_min_pulse[RIGHT_SERVO] + servo_max_pulse[RIGHT_SERVO]) / 2.0f;
    currentPulseValues[BASE_SERVO] = (servo_min_pulse[BASE_SERVO] + servo_max_pulse[BASE_SERVO]) / 2.0f;
    currentPulseValues[CLAW_SERVO] = (servo_min_pulse[CLAW_SERVO] + servo_max_pulse[CLAW_SERVO]) / 2.0f;

    resetHistoryValues(leftValues, 0);
    resetHistoryValues(rightValues, 0);
    resetHistoryValues(baseValues, 0);
    resetHistoryValues(clawValues, 0);
}

void setupPSX2()
{
    Serial.println("PS2X");
    int error = ps2x.config_gamepad(
        PSX2_CLOCK,
        PSX2_COMMAND,
        PSX2_ATTENTION,
        PSX2_DATA,
        PSX2_PRESSURES,
        PSX2_RUMBLE);

    if (error == 1)
    {
        Serial.println("No controller found, check wiring.");
    }
    else if (error == 2)
    {
        Serial.println("Controller found but not accepting commands.");
    }
    else if (error == 3)
    {
        Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
    }

    uint8_t type = ps2x.readType();
    switch (type) {
    case 0:
        Serial.println("Unknown Controller type");
        break;
    case 1:
        Serial.println("DualShock Controller Found");
        break;
    case 2:
        Serial.println("GuitarHero Controller Found");
        break;
    }
}

void setup()
{
	Serial.println("Init");
	Serial.begin(9600);
	pwm.begin();
	pwm.setPWMFreq(TOWERPRO_MG90S_FREQ);
	resetAllValues();
	setupPSX2();
}

void setPulseLength(uint8_t servonum, uint16_t pulselen)
{
  pwm.setPWM(pwmPins[servonum], 0, pulselen);
}

void servo_setMin(uint8_t servonum)
{
  setPulseLength(servonum, servo_min_pulse[servonum]);
}

void servo_setMax(uint8_t servonum)
{
  setPulseLength(servonum, servo_max_pulse[servonum]);
}

void servo_setMid(uint8_t servonum)
{
  setPulseLength(servonum, (servo_min_pulse[servonum] + servo_max_pulse[servonum]) / 2);
}

void servo_sweep_up(uint8_t servonum)
{
  for (uint16_t pulselen = servo_min_pulse[servonum]; pulselen <= servo_max_pulse[servonum]; pulselen++) 
  {
    setPulseLength(servonum, pulselen);    
  }
}

void servo_sweep_down(uint8_t servonum)
{
  for (uint16_t pulselen = servo_max_pulse[servonum]; pulselen >= servo_min_pulse[servonum]; pulselen--) 
  {
    setPulseLength(servonum, pulselen);    
  }
}

void incrementReadIndex()
{
    readIndex++;
    if (readIndex >= HISTORY)
    {
        readIndex = 0;
    }
}

int8_t getAverageHistoryValue(int8_t values[HISTORY])
{
	if (HISTORY == 1)
	{
		return values[0];
	}

    float sum = 0;
    for (uint8_t i = 0; i < HISTORY; ++i)
    {
        sum += values[i];
    }
    return (int8_t)(sum / (HISTORY * 1.0f));
}

uint8_t resetCounter = 0;



void readValuesPS2X()
{
    ps2x.read_gamepad(false, 0);

    if (ps2x.Button(PSB_CIRCLE))
    {
        resetCounter++;

        if (resetCounter >= 100)
        {
            resetAllValues();
            resetCounter = 0;
        }
    }
    else
    {
        resetCounter = 0;
    }

    if (ps2x.Button(PSB_TRIANGLE))
    {
        turbo = 3.0f;
    } 
    else if (ps2x.Button(PSB_CROSS))
    {
        turbo = 0.3f;
    }
    else
    {
        turbo = 1.0f;
    }

    int ry = ps2x.Analog(PSS_RY);
    int8_t right = map(ry, 0, 255, 5, -6);

    int rx = ps2x.Analog(PSS_RX);
    int8_t base1 = map(rx, 0, 255, 5, -6);

    uint8_t ly = ps2x.Analog(PSS_LY);
    int8_t left = map(ly, 255, 0, -5, 6);

    uint8_t lx = ps2x.Analog(PSS_LX);
    int8_t base2 = map(lx, 0, 255, 5, -5);

    int8_t claw = 0.0;
    if (ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1))
    {
        claw = -5;
    }
    else if (ps2x.Button(PSB_L2) || ps2x.Button(PSB_R2))
    {
        claw = 5;
    }

    //Serial.print(base);
    //Serial.print(' ');

    leftValues[readIndex] = left;
    rightValues[readIndex] = right;
    baseValues[readIndex] = base1 + base2;
    clawValues[readIndex] = claw;
    
    incrementReadIndex();
}

void setAllPulseLengths(uint16_t pulseValues[4])
{
    for (uint8_t servo = 0; servo < 4; ++servo)
    {
        if (prevPulseValues[servo] != pulseValues[servo])
        {
			//Serial.print(F("Servo "));
			//Serial.print(servo);
			//Serial.print(F(" "));
			//Serial.println(pulseValues[servo]);
            prevPulseValues[servo] = pulseValues[servo];
            setPulseLength(servo, pulseValues[servo]);
        }
    }
}

void applyToCurrentPulseValue(uint8_t servo, int8_t value)
{
    float currentValue = currentPulseValues[servo];

    currentValue += value * turbo / valueScales[servo];
    // clip to min max
    if (currentValue >= servo_max_pulse[servo])
    {
        currentValue = servo_max_pulse[servo];
    }

    if (currentValue <= servo_min_pulse[servo])
    {
        currentValue = servo_min_pulse[servo];
    }

    currentPulseValues[servo] = currentValue;
}

void loop()
{
	readValuesPS2X();

	float left = getAverageHistoryValue(leftValues);
	float right = getAverageHistoryValue(rightValues);
	float base = getAverageHistoryValue(baseValues);
	float claw = getAverageHistoryValue(clawValues);

	applyToCurrentPulseValue(LEFT_SERVO, left);
	applyToCurrentPulseValue(RIGHT_SERVO, right);
	applyToCurrentPulseValue(BASE_SERVO, base);
	applyToCurrentPulseValue(CLAW_SERVO, claw);

	uint16_t pulseValues[4] = { 0, 0, 0, 0 };
	for (uint8_t servo = 0; servo < 4; ++servo)
	{
		pulseValues[servo] = (uint16_t)currentPulseValues[servo];
	}

	setAllPulseLengths(pulseValues);
}
