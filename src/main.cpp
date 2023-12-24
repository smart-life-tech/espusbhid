#include "USB.h"
#include "ESP32_flight_stick.h"
#include "USBHIDKeyboard.h"
#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>
#include <Bounce2.h>
#include "OneButton.h"

ESP32_flight_stick FSJoy;
const int buttonCount = 8;
int Cruise_Climb = LOW;
// save the millis when a press has started.
unsigned long HoldCenterTime;
boolean Joy_Inactive = true;
int Joy_Active_Counter = 0;
const int Up_Pin = 15;       // UP         (Joystick Up)
const int Down_Pin = 38;     // Down       (Joystick Down)
const int Left_Pin = 37;     // Left       (Joystick Left)
const int Right_Pin = 39;    // Right      (Joystick Right)
const int Center_Pin = 15;   // Enter      (Joystick Press)
const int Rectangle_Pin = 5; // Rectangle  (Button 2)
const int Triangle_Pin = 6;  // Triangle   (Button 3)
const int Circle_Pin = 7;    // Circle     (Button 4)
const int Cancel_Pin = 8;    // X          (Button 5)

// Button's keys
const char Up_Press_Key = KEY_UP_ARROW;
const char Down_Press_Key = KEY_DOWN_ARROW;
const char Left_Press_Key = KEY_LEFT_ARROW;
const char Right_Press_Key = KEY_RIGHT_ARROW;

const char Rectangle_Press_Key = FSJOYSTICK_DPAD_UP;
const char Rectangle_Hold_Key = FSJOYSTICK_DPAD_UP_RIGHT;
const char Triangle_Press_Key = FSJOYSTICK_DPAD_RIGHT;
const char Triangle_Hold_Key = FSJOYSTICK_DPAD_DOWN_RIGHT;
const char Circle_Press_Key = FSJOYSTICK_DPAD_DOWN;
const char Circle_Hold_Key = FSJOYSTICK_DPAD_DOWN_LEFT;
const char Cancel_Press_Key = FSJOYSTICK_DPAD_LEFT;
const char Cancel_Hold_Key = FSJOYSTICK_DPAD_UP_LEFT;

const char ret = KEY_RETURN;

const int Joy_Rebounce_Interval = 3;
const int Joy_Rebounce_Threshold = 20;
const int Joy_Active_Threshold = 100;
const int Button_Hold_Threshold = 500;
const int Button_Rebounce_Interval = 500;
const int Reset_Threshold = 5000;
const int Joy_Hold_Threshold = 1;

// PushButton's instances
PushButton Up = PushButton(Up_Pin);
PushButton Down = PushButton(Down_Pin);
PushButton Left = PushButton(Left_Pin);
PushButton Right = PushButton(Right_Pin);
PushButton Center = PushButton(Center_Pin);
PushButton Rectangle = PushButton(Rectangle_Pin);
PushButton Triangle = PushButton(Triangle_Pin);
PushButton Circle = PushButton(Circle_Pin);
PushButton Cancel = PushButton(Cancel_Pin);

// OneButton buttons[buttonCount];
OneButton buttons(Center_Pin, true);

void keyboardPress(char key)
{
    if (key == Up_Press_Key || key == Down_Press_Key || key == Left_Press_Key || key == Right_Press_Key)
        FSJoy.press(key);
    else
    {
        FSJoy.dPad(key);
        FSJoy.write();
    }
}

void Button_onRelease(Button &btn, uint16_t duration)
{
    if (btn.is(Rectangle))
        keyboardPress(Rectangle_Press_Key);
    if (btn.is(Triangle))
        keyboardPress(Triangle_Press_Key);
    if (btn.is(Circle))
        keyboardPress(Circle_Press_Key);
    if (btn.is(Cancel))
        keyboardPress(Cancel_Press_Key);
    FSJoy.releaseAll();
}

void Button_onHold(Button &btn, uint16_t duration)
{
    if (btn.is(Rectangle))
        keyboardPress(Rectangle_Hold_Key);
    if (btn.is(Triangle))
        keyboardPress(Triangle_Hold_Key);
    if (btn.is(Circle))
        keyboardPress(Circle_Hold_Key);
    FSJoy.releaseAll();
}

void Joy_onHoldRepeat(Button &btn, uint16_t duration, uint16_t repeat_count)
{
    if (btn.isPressed() && Joy_Active_Counter == 5)
    {
        if (btn.is(Up))
            keyboardPress(Up_Press_Key);
        if (btn.is(Down))
            keyboardPress(Down_Press_Key);
        if (btn.is(Left))
            keyboardPress(Left_Press_Key);
        if (btn.is(Right))
            keyboardPress(Right_Press_Key);
        FSJoy.releaseAll();
    }
    Joy_Active_Counter = Joy_Active_Counter + 1;
    if (Joy_Inactive && Joy_Active_Counter > Joy_Active_Threshold)
    {
        Joy_Active_Counter = 0;
        Joy_Inactive = false;
    }
    if (!Joy_Inactive && Joy_Active_Counter > Joy_Rebounce_Threshold)
        Joy_Active_Counter = 0;
}
void Joy_onRelease(Button &btn, uint16_t duration)
{
    Joy_Active_Counter = 0;
    Joy_Inactive = true;
}

void SingleClick()
{ // this function will be called when the Joy center button is pressed 1 time only
    FSJoy.dPad(FSJOYSTICK_DPAD_CENTERED);
    FSJoy.write();
    Serial.println("SingleClick() detected.");
} // SingleClick

void DoubleClick()
{ // this function will be called when the Joy center button was pressed 2 times in a short timeframe.
    if (Cruise_Climb == LOW)
    {
        FSJoy.press('V');
        Serial.println("Vario");
    }
    else
    {
        FSJoy.press('S');
        Serial.println("Speed to fly");
    }
    Cruise_Climb = !Cruise_Climb; // reverse the Cruise_Climb
} // DoubleClick

void HoldCenter()
{ // this function will be called when the Joy center button is held down for 0.5 second or more.
    FSJoy.press('P');
    Serial.println("PAN()");
    HoldCenterTime = millis() - 500; // as set in setPressTicks()
} // HoldCenter()

void Button_onHoldRepeat(Button &btn, uint16_t duration, uint16_t repeat_count)
{ // this function will be called when the Cancel button is held down for a longer time
    if (btn.is(Cancel))
    {
        if (repeat_count == 1)
        {
            keyboardPress(Cancel_Hold_Key);
            FSJoy.releaseAll();
        }
        if (duration > 5000)
        {
            FSJoy.press('E');
            delay(1000);
            ESP.restart(); // ESP32_Restart
        }
    }
}
void setup()
{
    buttons.attachClick(SingleClick);
    buttons.attachDoubleClick(DoubleClick);
    buttons.setPressMs(1000); // that is the time when LongHoldCenter is called
    buttons.attachLongPressStart(HoldCenter);

    FSJoy.begin();
    USB.begin();
    pinMode(Up_Pin, INPUT_PULLUP);
    pinMode(Down_Pin, INPUT_PULLUP);
    pinMode(Left_Pin, INPUT_PULLUP);
    pinMode(Right_Pin, INPUT_PULLUP);
    pinMode(Center_Pin, INPUT_PULLUP);
    pinMode(Rectangle_Pin, INPUT_PULLUP);
    pinMode(Triangle_Pin, INPUT_PULLUP);
    pinMode(Circle_Pin, INPUT_PULLUP);
    pinMode(Cancel_Pin, INPUT_PULLUP);

    Serial.begin(115200);

    Up.onRelease(Joy_onRelease);
    Down.onRelease(Joy_onRelease);
    Left.onRelease(Joy_onRelease);
    Right.onRelease(Joy_onRelease);

    Up.onHoldRepeat(Joy_Hold_Threshold, Joy_Rebounce_Interval, Joy_onHoldRepeat);
    Down.onHoldRepeat(Joy_Hold_Threshold, Joy_Rebounce_Interval, Joy_onHoldRepeat);
    Left.onHoldRepeat(Joy_Hold_Threshold, Joy_Rebounce_Interval, Joy_onHoldRepeat);
    Right.onHoldRepeat(Joy_Hold_Threshold, Joy_Rebounce_Interval, Joy_onHoldRepeat);

    Center.onRelease(0, Button_Hold_Threshold - 1, Button_onRelease);
    Rectangle.onRelease(0, Button_Hold_Threshold - 1, Button_onRelease);
    Triangle.onRelease(0, Button_Hold_Threshold - 1, Button_onRelease);
    Circle.onRelease(0, Button_Hold_Threshold - 1, Button_onRelease);
    Cancel.onRelease(0, Button_Hold_Threshold - 1, Button_onRelease);

    Center.onHold(Button_Hold_Threshold, Button_onHold);
    Rectangle.onHold(Button_Hold_Threshold, Button_onHold);
    Triangle.onHold(Button_Hold_Threshold, Button_onHold);
    Circle.onHold(Button_Hold_Threshold, Button_onHold);
    Cancel.onHoldRepeat(Button_Hold_Threshold, Button_Rebounce_Interval, Button_onHoldRepeat);

    Joy_Active_Counter = 0;
}

void loop()
{
    buttons.tick();
    Up.update();
    Down.update();
    Left.update();
    Right.update();
    Center.update();
    Rectangle.update();
    Triangle.update();
    Circle.update();
    Cancel.update();
    /*
        static uint8_t count = FSBUTTON_0;
        if (count > FSBUTTON_11)
        {
            FSJoy.releaseAll();
            count = FSBUTTON_0;
        }
        FSJoy.press(count);
        count++;

    // Go through all dPad positions
    static uint8_t dpad = FSJOYSTICK_DPAD_UP;
    FSJoy.dPad(dpad++);
    if (dpad > FSJOYSTICK_DPAD_UP_LEFT)
        dpad = FSJOYSTICK_DPAD_UP;

    // Functions above only set the values.
    // This writes the report to the host.
    FSJoy.write();
*/
    delay(100);
}