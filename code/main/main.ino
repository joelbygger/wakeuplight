

// include the library code:
#include <LiquidCrystal.h>


// Clock.
const int intPinPWMOutput = 10; // 16 bit PWM channel, if configured so.
const int intPinPWMInput = 2;

const int hourPosOnDisplay = 7;
const int minPosOnDisplay = 10;
const int secPosOnDisplay = 13;

static volatile int priv_intCounter = 0;             // counts rising edge clock signals
static volatile int priv_intCompensator = 0;         // compensates for clock inaccuracy, freq. measured to be: 490.2Hz
const int intComepnsatorLimit = 5;
typedef struct
{
    int seconds;
    int minutes;
    int hours;
}time_t;
typedef enum
{
    alarmState_inactive,
    alarmState_active
}alarmState_t;
static time_t priv_currTime;
static bool priv_cursorOn = false;
static bool priv_time_moveCursor = false;
const unsigned long cursor_disable_timeout = 6000; // [ms]
// We seem to need this, otherwise cursor will flicker.
const unsigned long cursor_update_interval = 100; // [ms]
const int ALARM_START_TIME_HOUR = 00;
const int ALARM_START_TIME_MIN = 00;

// Display
const int led_pin_rs = 9;
const int led_pin_en = 8;
const int led_pin_d4 = 16;
const int led_pin_d5 = 6;
const int led_pin_d6 = 5;
const int led_pin_d7 = 4;

// The lamp itself.
const int ledPin = 3;
const unsigned int ledLinearity[31] = {
    0, 1, 2, 3, 4, 6, 8, 10, 13, 16, 
    20, 24, 29, 34, 40, 47, 54, 63, 72, 82, 
    92, 104, 116, 130, 145, 160, 177, 195, 214, 234, 
    255
};

// Joystick.
typedef enum
{
    sw_press_typeNone,
    sw_press_veryLongDelay,
    sw_press_longDelay,
    sw_press_shortDelay,
    sw_press_unparsable
} joySWstates_t;

const int joyPinX = A0;
const int joyPinY = A1;
const int joySWpin = 7;
static int priv_joyReadX = 0;
static int priv_joyReadY = 0;
static volatile unsigned long priv_joySWdebounceTime_last = 0xFFFFFFF;
static volatile unsigned long priv_lastTimeJoyMovement = 0;
volatile joySWstates_t priv_joySWpress = sw_press_typeNone;


const unsigned long JOY_AXIS_READ_DELAY = 100; // [ms], we don't want to read too often. 
const unsigned long JOY_AXIS_USE_DELAY = 400; // [ms], we don't want to use samples too often.
// The times seems to be difficult to set, so use big differences.
const unsigned long JOY_SW_SHORT_DELAY = 50; // [ms] I am not usre if this is real, sounds like short time, but works better than 100.
const unsigned long JOY_SW_LONG_DELAY = 1000; // [ms] 
const unsigned long JOY_SW_VERY_LONG_DELAY = 4000; // [ms]
// [ADC] Smaller movements than this we don't count, abs val.
const int JOY_MIN_DIFF_TO_COUNT = 200;
const int JOY_IN_REST = 501; // Pretty much true, we have 2048 bits ADC.

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(led_pin_rs, led_pin_en, led_pin_d4, led_pin_d5, led_pin_d6, led_pin_d7);

/**
* Setup of HW.
*/
void setup()
{
    Serial.begin(57600);
    Serial.println("We begin!");

    // Display.
    lcd.begin(16, 2); // set up the LCD's number of columns and rows.
    lcd.clear(); // start with a blank screen.
    lcd.setCursor(0,0); // set cursor to column 0, row 0 (the first row).
    
    // Clock.
    priv_currTime.seconds = 0;
    priv_currTime.minutes = 0;
    priv_currTime.hours = 0;
    priv_lastTimeJoyMovement = 0;
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
    attachInterrupt(digitalPinToInterrupt(joySWpin), joy_SWpressed, CHANGE);
}

/**
* Interrupt for the clock signal.
*/
void IntClockCounter()      // called by interrupt
{
    priv_intCounter++;

    if (priv_intCompensator >= intComepnsatorLimit)
    {
        priv_intCompensator = 0;
        priv_intCounter--;
    }
    
    if (priv_intCounter == 490) // 490Hz reached
    {  
        priv_intCompensator++;
    
        priv_currTime.seconds ++; // Don't update in general function, update here, faster and will work good enough.
        priv_intCounter = 0;     // Reset after 1 second is reached
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

/*
 * General clock update functions.
 */
void incrementSeconds()
{
    priv_currTime.seconds++;
}
void decrementSeconds()
{
    priv_currTime.seconds--;
}

void incrementMinutes()
{
    priv_currTime.minutes++;
}
void decrementMinutes()
{
    priv_currTime.minutes--;
}

void incrementHours()
{
    priv_currTime.hours++;
}
void decrementHours()
{
    priv_currTime.hours--;
}

/**
* Upcates and compaensates clock.
*
* NOTE: This function only works if no param changes > 59 ticks between calls.
*
*/
void updateClock(time_t &time)
{
    if (time.seconds >= 60)
    {
        incrementMinutes();
        time.seconds = 0;
    }
    if(time.minutes >= 60)
    {
        incrementHours();
        time.minutes = 0;
    }
    if(time.hours >= 24)
    {
        time.hours = 0;
    }
}

/**
 * Turns LED on and off.
 */
void controlLED(const time_t time)
{ 
    static int lastAlarmMinute = 99; // Something more than a legal minute.
    static uint8_t alarmMinute = 0;
    static alarmState_t alarmState = alarmState_inactive; 

    switch (alarmState)
    {
        case alarmState_inactive:
            if ((time.hours == ALARM_START_TIME_HOUR) &&
                (time.minutes == ALARM_START_TIME_MIN))
            {
                Serial.print("Alarm started ");
                alarmMinute = 0;
                lastAlarmMinute = time.minutes;

                alarmState = alarmState_active;
            }
            break;

        case alarmState_active:
        {
            if (lastAlarmMinute != time.minutes)
            {
                int pwmVal_ = ledLinearity[alarmMinute];
                analogWrite(ledPin, pwmVal_);

                lastAlarmMinute = time.minutes;
                alarmMinute++;

                if ( alarmMinute > ((sizeof(ledLinearity) / sizeof(ledLinearity[0]) - 1)) )
                {
                    Serial.print(" alarm reset ");
                    alarmMinute = 0;

                    analogWrite(ledPin, 0);
                    alarmState = alarmState_inactive;
                }
            }
            break;
        }

        default:
            alarmState = alarmState_inactive;
            break;
    }
}

/**
 * Returns next cursor position.
 */
int doMoveCursor(const int currCursorPos)
{
    int result = currCursorPos;

    switch(currCursorPos)
    {
        case hourPosOnDisplay:
            Serial.println("Cursor set to mins");
            result = minPosOnDisplay;
            break;
        case minPosOnDisplay:
            Serial.println("Cursor set to secs");
            result = secPosOnDisplay;
            break;
        case secPosOnDisplay:
        default:
            Serial.println("Cursor set to hours");
            // Handle all strange stuff as hour pos.
            result = hourPosOnDisplay;
            break;
    }

    return result;
}

/**
 * Updates cursor pos, moves cursor and 
 * deactivates curor if no movement for too long time.
 */
bool updateCursorPos(
    const bool cursorActive, 
    bool &moveCursor,
    int &cursorPos)
{
    static bool firstExec = true;
    static unsigned long lastUpdateTime = 0;
    const unsigned long currTime = millis();
    static bool lastCallCursorWasActive = false;
    bool cursorStillActive = cursorActive;

    // Some init.
    if (firstExec)
    {
        lastUpdateTime = currTime;
        firstExec = false;
        cursorPos = doMoveCursor(255); // Some strange number to reset position.
        moveCursor = false;
    }

    // Shall cursor be deactivated due to timeout?
    if (cursorStillActive &&
        ((currTime - priv_lastTimeJoyMovement) > cursor_disable_timeout))
    {
        cursorStillActive = false;
    }

    // If we become active, set cursor to start pos.
    if (!lastCallCursorWasActive && cursorStillActive)
    {
        cursorPos = doMoveCursor(255); // Some strange number to reset position.
        moveCursor = false;
    }
    lastCallCursorWasActive = cursorStillActive;

    if((currTime - lastUpdateTime) > cursor_update_interval)
    {
        if (cursorStillActive)
        {
            if (moveCursor)
            {
                lcd.setCursor(cursorPos, 1);
                lcd.write(' ');
                cursorPos = doMoveCursor(cursorPos);
                moveCursor = false;
            }

            lcd.setCursor(cursorPos, 1);
            lcd.write('I');
            lastUpdateTime = currTime;

            //Serial.print("currTime ");
            //Serial.print(currTime);
            //Serial.print(" priv_lastTimeJoyMovement ");
            //Serial.print(priv_lastTimeJoyMovement);
            //Serial.print(" diff ");
            //Serial.println(currTime - priv_lastTimeJoyMovement);
        }
        else
        {
            //Serial.println("Disable cursor");
            lcd.setCursor(cursorPos, 1);
            lcd.write(' ');
        }
    }

    return cursorStillActive;
}

/**
 * Returns how the time shall be modified, add or remove 1.
 */
void manualTimeChange(void (*increment)(), void (*decrement)())
{
    if (joy_xAxisPos())
    {
        increment();
    }
    else if (joy_xAxisNeg())
    {
        decrement();
    }
}

/*
 * Manages display, time etc. based on user input.
 */
void handleUserInput(
    const bool cursorActive, 
    bool &moveCursor)
{
    static int cursorPos = hourPosOnDisplay;
    unsigned long currTime = millis();
    static unsigned long lastTimeCheckedInput = 0;
    static bool firstExec = true;

    if (firstExec)
    {
        lastTimeCheckedInput = millis();
        firstExec = false;
    } 

    priv_cursorOn = updateCursorPos(
        cursorActive,
        moveCursor,
        cursorPos);


    if (priv_cursorOn &&
        ((currTime - lastTimeCheckedInput) >= JOY_AXIS_USE_DELAY))
    {
        switch(cursorPos)
        {
            case hourPosOnDisplay:
                manualTimeChange(&incrementHours, &decrementHours);
                break;
            case minPosOnDisplay:
                manualTimeChange(&incrementMinutes , &decrementMinutes);
                break;
            case secPosOnDisplay:
                manualTimeChange(&incrementSeconds, &decrementSeconds);
                break;
            default:
                // Well, we shouldn't be here.
                Serial.println("Handle user input has bad info about cursor pos!"); 
                break;
        }

        lastTimeCheckedInput = currTime;
    }
}

/**
* Updates display.
*/
void updateDisplay(const time_t time)
{   
    lcd.setCursor(0, 0);
    printTime("Time: ", time.hours, time.minutes, time.seconds);
}

/**
 * Returns true if movement was so big we count is as a movement.
 */
bool joy_readMovmentCounts(const int last, const int curr)
{
    bool result = false;

    if(last > curr)
    {
        result = (last - curr) > JOY_MIN_DIFF_TO_COUNT ? true : false;
    }
    else if(last < curr)
    {
        result = (curr- last) > JOY_MIN_DIFF_TO_COUNT ? true : false;
    }
    else
    {
        result = false;
    }

    return result;
}

/**
 * Read analog values from joystick.
 */
void joy_read()
{
    static bool shouldReadX = true;
    static unsigned long lastReadMilli = 0;
    static int lastReadX = 0;
    static int lastReadY = 0;

    if ( (millis() - lastReadMilli) > JOY_AXIS_READ_DELAY )
    {
        if (shouldReadX)
        {
            priv_joyReadX = analogRead(joyPinX);

            if (joy_readMovmentCounts(lastReadX, priv_joyReadX) ||
                (priv_joyReadX >= (JOY_IN_REST + JOY_MIN_DIFF_TO_COUNT)) ||
                (priv_joyReadX < (JOY_IN_REST - JOY_MIN_DIFF_TO_COUNT)))
            {
                priv_lastTimeJoyMovement = millis();
            }

            lastReadX = priv_joyReadX;
        }
        else
        {
            priv_joyReadY = analogRead(joyPinY);

            if (joy_readMovmentCounts(lastReadY, priv_joyReadY) ||
                (priv_joyReadY >= (JOY_IN_REST + JOY_MIN_DIFF_TO_COUNT)) ||
                (priv_joyReadY < (JOY_IN_REST - JOY_MIN_DIFF_TO_COUNT)))
            {
                priv_lastTimeJoyMovement = millis();
            }

            lastReadY = priv_joyReadY;
        }

        shouldReadX = !shouldReadX;
        lastReadMilli = millis();
    }
}

/*
 * Returns true if X axis reading was positive last time it was read. 
 */
bool joy_xAxisPos()
{
    return priv_joyReadX >= (JOY_IN_REST + JOY_MIN_DIFF_TO_COUNT) ? true : false;
}

/*
 * Returns true if X axis reading was negative last time it was read. 
 */
bool joy_xAxisNeg()
{
    return priv_joyReadX < (JOY_IN_REST - JOY_MIN_DIFF_TO_COUNT) ? true : false;
}

/**
* Interrupt routine for button.
*/
void joy_SWpressed()
{
    unsigned long currTime = millis();
    unsigned long timeDiff = currTime - priv_joySWdebounceTime_last;
    priv_lastTimeJoyMovement = millis();

    if (timeDiff < JOY_SW_SHORT_DELAY)
    {
        // Ignore, treat it as debounce.
        Serial.print("Debounce ");
        Serial.println(timeDiff);
    }
    else if (digitalRead(joySWpin) == LOW)
    {
        // Pressed, i.e. signal is low.
        Serial.print("Depressed " );
        Serial.println(currTime);
        priv_joySWdebounceTime_last = currTime;
    }
    else
    {
        // Released.
        Serial.print("Release " );
        Serial.print(currTime);
        if (timeDiff >= JOY_SW_VERY_LONG_DELAY)
        {
            Serial.print(" Very long ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_veryLongDelay;
        }
        else if (timeDiff >= JOY_SW_LONG_DELAY)
        {
            Serial.print(" Long ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_longDelay;
        }
        else if (timeDiff >= JOY_SW_SHORT_DELAY)
        {
            Serial.print(" Short ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_shortDelay;
        }
        else
        {
            Serial.print(" Unparsable ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_unparsable;
        }
    }   
}

/**
* Our main looptiloop.
*/
void loop()
{
    joy_read();

    handleUserInput(priv_cursorOn, priv_time_moveCursor);

    updateClock(priv_currTime);

    controlLED(priv_currTime);

    updateDisplay(priv_currTime);

    switch (priv_joySWpress)
    {
        case sw_press_typeNone:
            // Do nothing.
            break;
        case sw_press_veryLongDelay:
            break;
        case sw_press_longDelay:
            priv_cursorOn = !priv_cursorOn;
            break;
        case sw_press_shortDelay:
            priv_time_moveCursor = true;
            break;
        case sw_press_unparsable:
            break;
        default:
            // Do nothing
            break;
    }

    priv_joySWpress = sw_press_typeNone;
}
