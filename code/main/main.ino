

// include the library code:
#include <LiquidCrystal.h>


// Clock.
const int intPinPWMOutput = 10; // 16 bit PWM channel, if configured so.
const int intPinPWMInput = 2;
int intCounter = 0;             // counts rising edge clock signals
int intCompensator = 0;         // compensates for clock inaccuracy, freq. measured to be: 490.2Hz
const int intComepnsatorLimit = 5;
int intSeconds = 0;
int intMinutes = 0;
int intHours = 0;

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
typedef enum
{
    sw_press_none,
    sw_press_short,
    sw_press_long,
    sw_press_setTime
} joySWstates_t;

const int joyPinX = A0;
const int joyPinY = A1;
const int joySWpin = 7;
bool joyShouldReadX = true;
int joyReadX = 0;
int joyReadY = 0;
unsigned long joyReadAnalogDelay_last = 0;
unsigned long joySWdebounceTime_last = 0xFFFFFFFFFFFFFFFFFFFFFFFF;
joySWstates_t joySWpress = sw_press_none;

const unsigned long JOY_READ_ANALOG_DELAY = 100; // [ms], we don't want to read too often.
// The times seems to be difficult to set, so use big differences.
const unsigned long JOY_SW_SHORT_DELAY = 100; // [ms]
const unsigned long JOY_SW_LONG_DELAY = 1000; // [ms] 
const unsigned long JOY_SW_VERY_LONG_DELAY = 3000; // [ms] 

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(led_pin_rs, led_pin_en, led_pin_d4, led_pin_d5, led_pin_d6, led_pin_d7);

/**
* Setup of HW.
*/
void setup()
{
    // Display.
    lcd.begin(16, 2); // set up the LCD's number of columns and rows.
    lcd.clear(); // start with a blank screen.
    lcd.setCursor(0,0); // set cursor to column 0, row 0 (the first row).
    
    // Clock.
    //  intPinPWMInput is our interrupt, IntClockCounter function is called when
    //  invoked on a RISING clock edge
    attachInterrupt(digitalPinToInterrupt(intPinPWMInput), IntClockCounter, RISING);
    analogReference(DEFAULT);
    pinMode(intPinPWMOutput, OUTPUT);
    analogWrite(intPinPWMOutput, 127);   // this starts our PWM 'clock' with a 50% duty cycle
    
    // The lamp itself.
    pinMode(ledPin, OUTPUT);

    // Joystick.
    // No need to init the analog reads of the X & Y axis.
    pinMode(joySWpin, INPUT_PULLUP); // We need a pullup, use MCU internal.
    attachInterrupt(digitalPinToInterrupt(joySWpin), joySWpressed, CHANGE);
}

/**
* Interrupt for the clock signal.
*/
void IntClockCounter()      // called by interrupt
{
    intCounter ++;        // with each clock rise add 1 to intCounter count

    if (intCompensator >= intComepnsatorLimit)
    {
        intCompensator = 0;
        intCounter--;
    }
    
    if (intCounter == 490) // 490Hz reached
    {  
        intCompensator++;
    
        intSeconds ++;          // after one 490Hz cycle add 1 second ;)
        intCounter = 0;     // Reset after 1 second is reached
    }
    
    return;
}


/**
* Prints stuff to display.
*/
void printTime(String txt, int hrs, int mins, int secs)
{
    lcd.print(txt);
    if(hrs <= 9)
    {
        lcd.print("0");
    }
    lcd.print(hrs);
    lcd.print(":");
    if(mins <= 9)
    {
        lcd.print("0");
    }
    lcd.print(mins);
    lcd.print(":");
    if(secs <= 9)
    {
        lcd.print("0");
    }
    lcd.print(secs);
}


/**
* Upcates and compaensates clock.
*/
void updateClock()
{
    if (intSeconds == 60)
    {
        intMinutes++;
        intSeconds = 0;
    }
    if(intMinutes == 60)
    {
        intHours++;
        intMinutes = 0;
    }
    if(intHours == 24)
    {
        intHours = 0;
    }
}

/**
* Updates display.
*/
void updateDisplay(int hrs, int mins, int secs, const bool clockCanBeChanged)
{
    lcd.setCursor(0, 0);
    printTime("Time: ", hrs, mins, secs);
}

/**
 * Read analog values from joystick.
 */
void readJoy()
{
    if ( (millis() - joyReadAnalogDelay_last) > JOY_READ_ANALOG_DELAY )
    {
        if (joyShouldReadX)
        {
            joyReadX = analogRead(joyPinX);
        }
        else
        {
            joyReadY = analogRead(joyPinY);
        }

        joyShouldReadX = !joyShouldReadX;
        joyReadAnalogDelay_last = millis();
    }   
}

/**
* Interrupt routine for button.
*/
void joySWpressed()
{
    unsigned long currTime = millis();
    unsigned long timeDiff = currTime - joySWdebounceTime_last;

    // We are only interessed in presses, i.e. signal is low.
    if (digitalRead(joySWpin) == LOW)
    {
        joySWdebounceTime_last = currTime;
    }
    else
    {
        if (timeDiff > JOY_SW_VERY_LONG_DELAY)
        {
            joySWpress = sw_press_setTime;
        }
        else if (timeDiff > JOY_SW_LONG_DELAY)
        {
            joySWpress = sw_press_long;
        }
        else if (timeDiff > JOY_SW_SHORT_DELAY)
        {
            joySWpress = sw_press_short;
        }
        else
        {
            joySWpress = sw_press_none;
        }
    }   
}

/**
* Our main looptiloop.
*/
void loop()
{
    static bool clockCanBeChanged = false;

    readJoy();
  
    updateClock();

    updateDisplay(intHours, intMinutes, intSeconds, clockCanBeChanged);

    switch (joySWpress)
    {
        case sw_press_none:
            analogWrite(ledPin, 0);
            clockCanBeChanged = false;
            break;
        case sw_press_short:
            analogWrite(ledPin, 10);
            break;
        case sw_press_long:
            analogWrite(ledPin, 255);
            break;
        case sw_press_setTime:
            clockCanBeChanged = true;
            break;
        default:
            // Do nothing
            break;
    }

    if (clockCanBeChanged)
    {
        if (joyReadX > 1000)
        {
            analogWrite(ledPin, joyReadX/4 - 520);
        }
        else
        {
            analogWrite(ledPin, 0);
        }
    }

    /*if (intSeconds % 10 == 0)
    {
        analogWrite(ledPin, 20);
    }
    else if (intSeconds % 10 == 1)
    {
        analogWrite(ledPin, 60);
    }
    else if (intSeconds % 10 == 2)
    {
        analogWrite(ledPin, 100);
    }
    else if (intSeconds % 10 == 3)
    {
        analogWrite(ledPin, 160);
    }
    else if (intSeconds % 10 == 4)
    {
        analogWrite(ledPin, 256);
    }
    else if (intSeconds % 10 == 5)
    {
        analogWrite(ledPin, 255);
    }
    else if (intSeconds % 10 == 6)
    {
        analogWrite(ledPin, 200);
    }
    else if (intSeconds % 10 == 7)
    {
        analogWrite(ledPin, 160);
    }
    else if (intSeconds % 10 == 8)
    {
        analogWrite(ledPin, 100);
    }
    else if (intSeconds % 10 == 9)
    {
        analogWrite(ledPin, 1);
    }*/
}
