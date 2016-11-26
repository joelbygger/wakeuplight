

// include the library code:
#include <LiquidCrystal.h>


// Clock.
const int intPinPWMOutput = 10; // 16 bit PWM channel, if configured so.
const int intPinPWMInput = 2;
static volatile int priv_intCounter = 0;             // counts rising edge clock signals
static volatile int priv_intCompensator = 0;         // compensates for clock inaccuracy, freq. measured to be: 490.2Hz
const int intComepnsatorLimit = 5;
typedef struct
{
    int seconds;
    int minutes;
    int hours;
}time_t;
static time_t priv_currTime;
static bool priv_time_activateCursor = false;
static bool priv_time_moveCursor = false;
const unsigned long cursor_disable_timeout = 6000; // [ms]
// We seem to need this, otherwise cursor will flicker.
const unsigned long cursor_update_interval = 100; // [ms]

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
    sw_press_typeNone,
    sw_press_veryLongDelay,
    sw_press_longDelay,
    sw_press_shortDelay,
    sw_press_unparsable
} joySWstates_t;

const int joyPinX = A0;
const int joyPinY = A1;
const int joySWpin = 7;
static int joyReadX = 0;
static int joyReadY = 0;
static volatile unsigned long priv_joySWdebounceTime_last = 0xFFFFFFF;
static volatile unsigned long priv_lastTimeJoyMovement = 0;
volatile joySWstates_t priv_joySWpress = sw_press_typeNone;


const unsigned long JOY_READ_ANALOG_DELAY = 100; // [ms], we don't want to read too often.
// The times seems to be difficult to set, so use big differences.
const unsigned long JOY_SW_SHORT_DELAY = 60; // [ms] I am not usre if this is real, sounds like short time, but works better than 100.
const unsigned long JOY_SW_LONG_DELAY = 1000; // [ms] 
const unsigned long JOY_SW_VERY_LONG_DELAY = 2000; // [ms] 
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
    while (!Serial) 
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println("Connected, we begin!");

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
    attachInterrupt(digitalPinToInterrupt(joySWpin), joySWpressed, CHANGE);
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
    
        priv_currTime.seconds ++;          // after one 490Hz cycle add 1 second ;)
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


/**
* Upcates and compaensates clock.
*/
void updateTime(time_t &time)
{
    if (time.seconds >= 60)
    {
        time.minutes++;
        time.seconds = 0;
    }
    if(time.minutes >= 60)
    {
        time.hours++;
        time.minutes = 0;
    }
    if(time.hours >= 24)
    {
        time.hours = 0;
    }
}

/**
 * Returns next cursor position.
 */
int doMoveCursor(const int currCursorPos)
{
    const int hourPos = 7;
    const int minPos = 10;
    const int secPos = 13;
    int result = currCursorPos;

    switch(currCursorPos)
    {
        case hourPos:
            result = minPos;
            break;
        case minPos:
            result = secPos;
            break;
        case secPos:
        default:
            // Handle all strange stuff as hour pos.
            result = hourPos;
            break;
    }

    return result;
}

/**
 * Updates cursor pos, moves cursor and 
 * deactivates curor if no movement for too long time.
 */
void updateCursorPos(bool &cursorActive, bool &moveCursor)
{
    static int cursorPos = 0;
    static bool firstExec = true;
    static unsigned long lastUpdateTime = 0;
    const unsigned long currTime = millis();
    static bool lastCallCursorWasActive = false;

    // Some init.
    if (firstExec)
    {
        lastUpdateTime = currTime;
        firstExec = false;
        cursorPos = doMoveCursor(255); // Some strange number to reset position.
    }

    // Shall cursor be deactivated due to timeout?
    if (cursorActive &&
        ((currTime - priv_lastTimeJoyMovement) > cursor_disable_timeout))
    {
        cursorActive = false;
    }

    // If we become active, set cursor to start pos.
    if (!lastCallCursorWasActive && cursorActive)
    {
        cursorPos = doMoveCursor(255); // Some strange number to reset position.
    }
    lastCallCursorWasActive = cursorActive;

    if((currTime - lastUpdateTime) > cursor_update_interval)
    {
        if (cursorActive)
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

            Serial.print("currTime ");
            Serial.print(currTime);
            Serial.print(" priv_lastTimeJoyMovement ");
            Serial.print(priv_lastTimeJoyMovement);
            Serial.print(" diff ");
            Serial.println(currTime - priv_lastTimeJoyMovement);
        }
        else
        {
            Serial.println("Disable cursor");
            lcd.setCursor(cursorPos, 1);
            lcd.write(' ');
        }
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
bool readJoyMovmentCounts(const int last, const int curr)
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
void readJoy()
{
    static bool shouldReadX = true;
    static unsigned long lastReadMilli = 0;
    static int lastReadX = 0;
    static int lastReadY = 0;

    if ( (millis() - lastReadMilli) > JOY_READ_ANALOG_DELAY )
    {
        if (shouldReadX)
        {
            joyReadX = analogRead(joyPinX);

            if (readJoyMovmentCounts(lastReadX, joyReadX) ||
                (joyReadX > (JOY_IN_REST + JOY_MIN_DIFF_TO_COUNT)) ||
                (joyReadX < (JOY_IN_REST - JOY_MIN_DIFF_TO_COUNT)))
            {
                priv_lastTimeJoyMovement = millis();
            }

            lastReadX = joyReadX;
        }
        else
        {
            joyReadY = analogRead(joyPinY);

            if (readJoyMovmentCounts(lastReadY, joyReadY) ||
                (joyReadY > (JOY_IN_REST + JOY_MIN_DIFF_TO_COUNT)) ||
                (joyReadY < (JOY_IN_REST - JOY_MIN_DIFF_TO_COUNT)))
            {
                priv_lastTimeJoyMovement = millis();
            }

            lastReadY = joyReadY;
        }

        shouldReadX = !shouldReadX;
        lastReadMilli = millis();
    }
}

/**
* Interrupt routine for button.
*/
void joySWpressed()
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
    readJoy();
  
    updateTime(priv_currTime);

    updateCursorPos(priv_time_activateCursor, priv_time_moveCursor);
    updateDisplay(priv_currTime);

    switch (priv_joySWpress)
    {
        case sw_press_typeNone:
            // Do nothing.
            break;
        case sw_press_veryLongDelay:
            break;
        case sw_press_longDelay:
            priv_time_activateCursor = !priv_time_activateCursor;
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

    //if (clockCanBeChanged)
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
}
