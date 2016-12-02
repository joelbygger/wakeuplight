


typedef enum
{
    sw_press_typeNone,
    sw_press_veryLongDelay,
    sw_press_longDelay,
    sw_press_shortDelay,
    sw_press_unparsable
} joySWstates_t;


const unsigned long axis_read_delay = 100; // [ms], we don't want to read too often. 
// The times seems to be difficult to set, so use big differences.
const unsigned long sw_press_short_delay = 50; // [ms] I am not usre if this is real, sounds like short time, but works better than 100.
const unsigned long sw_press_long_delay = 1000; // [ms] 
const unsigned long sw_press_very_long_delay = 4000; // [ms]
// [ADC] Smaller movements than this we don't count, abs val.
const int axis_min_movement_to_count = 200;
const int axis_default_value = 501; // Pretty much true, we have 2048 bits ADC.

static int priv_joyReadX = 0;
static int priv_joyReadY = 0;
static volatile unsigned long priv_joySWdebounceTime_last = 0xFFFFFFF;
static volatile unsigned long priv_lastTimeJoyMovementTime = 0;
static volatile joySWstates_t priv_joySWpress = sw_press_typeNone;



/**
 * Returns true if X axis reading was positive last time it was read. 
 */
bool xAxisPos()
{
    return priv_joyReadX >= (axis_default_value + axis_min_movement_to_count) ? true : false;
}

/**
 * Returns true if X axis reading was negative last time it was read. 
 */
bool xAxisNeg()
{
    return priv_joyReadX < (axis_default_value - axis_min_movement_to_count) ? true : false;
}

/**
 * Returns how the time shall be modified, add or remove 1.
 */
void manualTimeChange(void (*increment)(), void (*decrement)())
{
    if (xAxisPos())
    {
        increment();
    }
    else if (xAxisNeg())
    {
        decrement();
    }
}

/**
 * Returns true if movement was so big we count is as a movement.
 */
bool readMovmentCounts(const int last, const int curr)
{
    bool result = false;

    if(last > curr)
    {
        result = (last - curr) > axis_min_movement_to_count ? true : false;
    }
    else if(last < curr)
    {
        result = (curr- last) > axis_min_movement_to_count ? true : false;
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
void joy_readAxis()
{
    static bool shouldReadX = true;
    static unsigned long lastReadMilli = 0;
    static int lastReadX = 0;
    static int lastReadY = 0;

    if ( (millis() - lastReadMilli) > axis_read_delay )
    {
        if (shouldReadX)
        {
            priv_joyReadX = analogRead(joyPinX);

            if (readMovmentCounts(lastReadX, priv_joyReadX) ||
                (priv_joyReadX >= (axis_default_value + axis_min_movement_to_count)) ||
                (priv_joyReadX < (axis_default_value - axis_min_movement_to_count)))
            {
                priv_lastTimeJoyMovementTime = millis();
            }

            lastReadX = priv_joyReadX;
        }
        else
        {
            priv_joyReadY = analogRead(joyPinY);

            if (readMovmentCounts(lastReadY, priv_joyReadY) ||
                (priv_joyReadY >= (axis_default_value + axis_min_movement_to_count)) ||
                (priv_joyReadY < (axis_default_value - axis_min_movement_to_count)))
            {
                priv_lastTimeJoyMovementTime = millis();
            }

            lastReadY = priv_joyReadY;
        }

        shouldReadX = !shouldReadX;
        lastReadMilli = millis();
    }
}

/**
* Interrupt routine for button.
*/
void joy_pressedInterrupt()
{
    unsigned long currTime = millis();
    unsigned long timeDiff = currTime - priv_joySWdebounceTime_last;
    priv_lastTimeJoyMovementTime = millis();

    if (timeDiff < sw_press_short_delay)
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
        if (timeDiff >= sw_press_very_long_delay)
        {
            Serial.print(" Very long ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_veryLongDelay;
        }
        else if (timeDiff >= sw_press_long_delay)
        {
            Serial.print(" Long ");
            Serial.println(timeDiff);
            priv_joySWpress = sw_press_longDelay;
        }
        else if (timeDiff >= sw_press_short_delay)
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
 * Returns what a button press means.
 */
void joy_getPressedState(
    bool &cursorActive, 
    bool &moveCursor)
{

    switch (priv_joySWpress)
    {
        case sw_press_typeNone:
            // Do nothing.
            break;
        case sw_press_veryLongDelay:
            break;
        case sw_press_longDelay:
            cursorActive = !cursorActive;
            break;
        case sw_press_shortDelay:
            moveCursor = true;
            break;
        case sw_press_unparsable:
            break;
        default:
            // Do nothing
            break;
    }

    priv_joySWpress = sw_press_typeNone;
}

/**
 * Returns last time joystick was moved or pressed.
 */
unsigned long joy_getLastTimeJoyMovementTime()
{
	return priv_lastTimeJoyMovementTime;
}
