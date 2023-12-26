#include <Arduino.h>
#include "USB.h"
#include "USBHIDGamepad.h"
USBHIDGamepad Gamepad;
// Variables to track button press state and time
bool buttonPressed = false;
unsigned long buttonPressStartTime = 0;
unsigned long LONG_PRESS_DURATION = 1000;
unsigned long SHORT_PRESS_DURATION = 200;

// Define corresponding button IDs
#define BUTTON_RECTANGLE 1
#define BUTTON_CIRCLE 3
#define BUTTON_CANCEL 5
#define BUTTON_TRIANGLE 7
#define BUTTON_RECTANGLE_RELEASE 2
#define BUTTON_CIRCLE_RELEASE 4
#define BUTTON_CANCEL_RELEASE 6
#define BUTTON_TRIANGLE_RELEASE 8

const int HAT_UP_PIN = 18;
const int HAT_LEFT_PIN = 37;
const int HAT_DOWN_PIN = 38;
const int HAT_RIGHT_PIN = 39;
const int HAT_CENTER_PIN = 15;
const int Rectangle_Button_Pin = 7; // Rectangle  (Button 2)
const int Circle_Button_Pin = 42;   // Circle     (Button 4)
const int Cancel_Button_Pin = 45;   // X          (Button 5)
const int Triangle_Button_Pin = 1;  // Triangle   (Button 3)

/*
// Define corresponding hat positions
#define HAT_UP 0
#define HAT_UP_LEFT 1
#define HAT_LEFT 2
#define HAT_DOWN_LEFT 3
#define HAT_DOWN 4
#define HAT_DOWN_RIGHT 5
#define HAT_RIGHT 6
#define HAT_UP_RIGHT 7
#define HAT_CENTER 8
*/
// Function to handle button press
void handleButtonPress(uint8_t button)
{
    if (!buttonPressed)
    {
        // Button was just pressed, record start time
        buttonPressStartTime = millis();
        buttonPressed = true;
        Gamepad.pressButton(button); // Send initial press
    }
    else
    {
        // Button is being held, check for long press
        unsigned long currentMillis = millis();
        if (currentMillis - buttonPressStartTime > LONG_PRESS_DURATION)
        {
            // Long press detected, handle accordingly
            Gamepad.releaseButton(button);
            Gamepad.pressButton(button + 1); // Release initial press
            // Additional actions for long press
        }
    }
}

// Function to handle button release
void handleButtonRelease(uint8_t button)
{
    buttonPressed = false;
    Gamepad.releaseButton(button);
    Gamepad.releaseButton(button + 1);
    unsigned long currentMillis = millis();
    if (currentMillis - buttonPressStartTime < SHORT_PRESS_DURATION)
    {
        // Short press detected, handle accordingly
        // Additional actions for short press
    }
}
void updateHatSwitch()
{
    int hatPosition = HAT_CENTER; // Assume the hat is in the center initially

    // Check the combination of button presses for each hat position
    if (digitalRead(HAT_UP_PIN) == LOW && digitalRead(HAT_LEFT_PIN) == LOW)
    {
        hatPosition = HAT_UP_LEFT;
    }
    else if (digitalRead(HAT_UP_PIN) == LOW && digitalRead(HAT_RIGHT_PIN) == LOW)
    {
        hatPosition = HAT_UP_RIGHT;
    }
    else if (digitalRead(HAT_DOWN_PIN) == LOW && digitalRead(HAT_LEFT_PIN) == LOW)
    {
        hatPosition = HAT_DOWN_LEFT;
    }
    else if (digitalRead(HAT_DOWN_PIN) == LOW && digitalRead(HAT_RIGHT_PIN) == LOW)
    {
        hatPosition = HAT_DOWN_RIGHT;
    }
    else if (digitalRead(HAT_UP_PIN) == LOW)
    {
        hatPosition = HAT_UP;
    }
    else if (digitalRead(HAT_LEFT_PIN) == LOW)
    {
        hatPosition = HAT_LEFT;
    }
    else if (digitalRead(HAT_DOWN_PIN) == LOW)
    {
        hatPosition = HAT_DOWN;
    }
    else if (digitalRead(HAT_RIGHT_PIN) == LOW)
    {
        hatPosition = HAT_RIGHT;
    }

    // Update the hat position in the Gamepad
    Gamepad.hat(hatPosition);
}

void setup()
{
    Serial.begin(115200);
    Gamepad.begin();
    USB.begin();

    // Initialize button pins
    // Initialize button pins
    pinMode(HAT_UP_PIN, INPUT_PULLUP);
    pinMode(HAT_LEFT_PIN, INPUT_PULLUP);
    pinMode(HAT_DOWN_PIN, INPUT_PULLUP);
    pinMode(HAT_RIGHT_PIN, INPUT_PULLUP);
    pinMode(HAT_CENTER_PIN, INPUT_PULLUP);
    pinMode(Rectangle_Button_Pin, INPUT_PULLUP);
    pinMode(Circle_Button_Pin, INPUT_PULLUP);
    pinMode(Cancel_Button_Pin, INPUT_PULLUP);
    pinMode(Triangle_Button_Pin, INPUT_PULLUP);
}

void loop()
{
    // Check button state and call corresponding functions
    if (digitalRead(Rectangle_Button_Pin) == LOW)
    {
        handleButtonPress(BUTTON_RECTANGLE);
    }
    else
    {
        handleButtonRelease(BUTTON_RECTANGLE);
    }

    if (digitalRead(Circle_Button_Pin) == LOW)
    {
        handleButtonPress(BUTTON_CIRCLE);
    }
    else
    {
        handleButtonRelease(BUTTON_CIRCLE);
    }

    if (digitalRead(Cancel_Button_Pin) == LOW)
    {
        handleButtonPress(BUTTON_CANCEL);
    }
    else
    {
        handleButtonRelease(BUTTON_CANCEL);
    }

    if (digitalRead(Triangle_Button_Pin) == LOW)
    {
        handleButtonPress(BUTTON_TRIANGLE);
    }
    else
    {
        handleButtonRelease(BUTTON_TRIANGLE);
    }

    // Update hat switch
    updateHatSwitch();
}
