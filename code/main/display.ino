


const int hourPosOnDisplay = 7;
const int minPosOnDisplay = 10;
const int secPosOnDisplay = 13;
const unsigned long cursor_disable_timeout = 6000; // [ms]
// We seem to need this, otherwise cursor will flicker.
const unsigned long cursor_update_interval = 100; // [ms]
const unsigned long joystick_axis_use_read_val_delay = 400; // [ms], we don't want to use samples too often.

/**
 * Init package global variables and the display itself.
 */
void display_init()
{
    lcd.begin(16, 2); // set up the LCD's number of columns and rows.
    lcd.clear(); // start with a blank screen.
    lcd.setCursor(0,0); // set cursor to column 0, row 0 (the first row).
    
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
    int &cursorPos,
    const unsigned long lastTimeJoyMovement)
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
        ((currTime - lastTimeJoyMovement) > cursor_disable_timeout))
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
            //Serial.print(" lastTimeJoyMovement ");
            //Serial.print(lastTimeJoyMovement);
            //Serial.print(" diff ");
            //Serial.println(currTime - lastTimeJoyMovement);
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
* Updates display.
*/
void display_updateDisplay(const time_t time)
{   
    lcd.setCursor(0, 0);
    printTime("Time: ", time.hours, time.minutes, time.seconds);
}

/*
 * Manages display, time etc. based on user input.
 */
void display_handleUserInput(
    bool &cursorActive, 
    bool &moveCursor,
    const unsigned long lastTimeJoyMovement)
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

    cursorActive = updateCursorPos(
        cursorActive,
        moveCursor,
        cursorPos,
        lastTimeJoyMovement);


    if (cursorActive &&
        ((currTime - lastTimeCheckedInput) >= joystick_axis_use_read_val_delay))
    {
        switch(cursorPos)
        {
            case hourPosOnDisplay:
                manualTimeChange(&clock_incrementHours, &clock_decrementHours);
                break;
            case minPosOnDisplay:
                manualTimeChange(&clock_incrementMinutes , &clock_decrementMinutes);
                break;
            case secPosOnDisplay:
                manualTimeChange(&clock_incrementSeconds, &clock_decrementSeconds);
                break;
            default:
                // Well, we shouldn't be here.
                Serial.println("Handle user input has bad info about cursor pos!"); 
                break;
        }

        lastTimeCheckedInput = currTime;
    }
}