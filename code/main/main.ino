

// include the library code:
#include <LiquidCrystal.h>

const String weekdays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

// For some reason must be declared in main.
typedef struct
{
    uint32_t seconds;
    uint32_t minutes;
    uint32_t hours;
    uint8_t dayNo;
    String dayText;
}time_t;

// For some reason must be declared in main.
typedef enum
{
    hourPosOnDisplay,
    minPosOnDisplay,
    secPosOnDisplay,
    dayPosOnDisplay
}cursorPosOnDisplay_t;

// Clock.
const int gpio_PWMOutput = 10; // 16 bit PWM channel, if configured so.
const int gpio_PWMInput = 2;


// Display
const int led_pin_rs = 9;
const int led_pin_en = 8;
const int led_pin_d4 = 16;
const int led_pin_d5 = 6;
const int led_pin_d6 = 5;
const int led_pin_d7 = 4;

// The lamp itself.
const int ledPin = 3;

// Joystick.
const int joyPinX = A0;
const int joyPinY = A1;
const int joySWpin = 7;




// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(led_pin_rs, led_pin_en, led_pin_d4, led_pin_d5, led_pin_d6, led_pin_d7);

/**
* Setup of HW.
*/
void setup()
{
    Serial.begin(57600);
    Serial.println("We begin!");

    clock_init();

    // Display.
    display_init();

    //  gpio_PWMInput is our interrupt, IntClockCounter function is called when
    //  invoked on a RISING clock edge
    attachInterrupt(digitalPinToInterrupt(gpio_PWMInput), clock_interrupt, RISING);
    analogReference(DEFAULT);
    pinMode(gpio_PWMOutput, OUTPUT);
    analogWrite(gpio_PWMOutput, 127);   // this starts our PWM 'clock' with a 50% duty cycle
    
    // The lamp itself.
    pinMode(ledPin, OUTPUT);

    // Joystick.
    // No need to init the analog reads of the X & Y axis.
    pinMode(joySWpin, INPUT_PULLUP); // We need a pullup, use MCU internal.
    attachInterrupt(digitalPinToInterrupt(joySWpin), joy_pressedInterrupt, CHANGE);
}



/**
* Our main looptiloop.
*/
void loop()
{
    static bool display_moveCursor = false;
    static bool display_cursorActive = false;

    joy_readAxis();
    joy_getPressedState(display_cursorActive, display_moveCursor);
    unsigned long lastMovementTime = joy_getLastTimeJoyMovementTime();

    display_updateDisplay(
        clock_getTime(),
        display_cursorActive, 
        display_moveCursor,
        lastMovementTime);

    led_controlLED(clock_getTime());

}
