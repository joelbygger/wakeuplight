


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
void printTime(int hrs, int mins, int secs)
{
    lcd.print("Time: ");
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
 * Prints current day to display.
 */
void printDay(String day)
{
    lcd.print("Day: ");
    lcd.print(day);
}

/**
 * Returns next cursor position.
 */
cursorPosOnDisplay_t doMoveCursor(cursorPosOnDisplay_t currCursorPos)
{
    cursorPosOnDisplay_t result = currCursorPos;

    // We also clears the display here, to make sure that we will only display what is changed.
    lcd.clear();

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
            Serial.println("Cursor set to day");
            result = dayPosOnDisplay;
            break;
        case dayPosOnDisplay:
        default:
            Serial.println("Cursor set to hours");
            // Handle all strange stuff as hour pos.
            result = hourPosOnDisplay;
            break;
    }

    return result;
}

void printCursor(bool visibleCursor, cursorPosOnDisplay_t cursorPos)
{
    char cursor = visibleCursor ? '*' : ' ';
    switch (cursorPos)
    {
        case hourPosOnDisplay:
            lcd.setCursor(7, 1);
            lcd.write(cursor);
            break;
        case minPosOnDisplay:
            lcd.setCursor(10, 1);
            lcd.write(cursor);
            break;
        case secPosOnDisplay:
            lcd.setCursor(13, 1);
            lcd.write(cursor);
            break;
        case dayPosOnDisplay:
        default:
            lcd.setCursor(8, 2);
            lcd.write(cursor);
            break;
    }
}

/**
 * Does actual printing of time, invokes printing of cursor.
 */
void printToDisplay(
        bool cursorActive,
        cursorPosOnDisplay_t cursorPos,
        time_t time)
{
    if (cursorActive)
    {
        // If cursor is active we do not display everyting, or cursor will overwrite stuff.
        switch (cursorPos)
        {
            case hourPosOnDisplay:
            case minPosOnDisplay:
            case secPosOnDisplay:
                lcd.setCursor(0, 0);
                printTime(time.hours, time.minutes, time.seconds);
                break;
            case dayPosOnDisplay:
            default:
                lcd.setCursor(0, 1);
                printDay(time.dayText);
                break;
        }
        printCursor(cursorActive, cursorPos);
    }
    else
    {
        lcd.setCursor(0, 0);
        printTime(time.hours, time.minutes, time.seconds);
        lcd.setCursor(0, 1);
        printDay(time.dayText);
    }
}

/**
 * Updates cursor pos, prints cursor, moves cursor and 
 * deactivates curor if no movement for too long time.
 * And also prints date and time.
 */
bool updateDisplay(
    time_t time,
    bool cursorActive, 
    bool &moveCursor,
    cursorPosOnDisplay_t &cursorPos,
    unsigned long lastTimeJoyMovement)
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
        cursorPos = doMoveCursor(dayPosOnDisplay); // Init to hour pos after first key press.
        moveCursor = false;
    }

    // Was cursor deactivated due to key presS?
    if (lastCallCursorWasActive && !cursorStillActive)
    {
        printCursor(false, cursorPos); // Here we force-off the cursor indicator.
    }

    // Shall cursor be deactivated due to timeout?
    if (cursorStillActive &&
        ((currTime - lastTimeJoyMovement) > cursor_disable_timeout))
    {
        printCursor(false, cursorPos); // Here we force-off the cursor indocator.

        cursorStillActive = false;
    }

    // If we become active, set cursor to start pos.
    if (!lastCallCursorWasActive && cursorStillActive)
    {
        cursorPos = doMoveCursor(dayPosOnDisplay); // Init to hour pos after first key press.
        moveCursor = false;
    }
    lastCallCursorWasActive = cursorStillActive;

    if((currTime - lastUpdateTime) > cursor_update_interval)
    {
        if (cursorStillActive)
        {
            if (moveCursor)
            {
                printCursor(false, cursorPos); // Here we force-off the cursor.
                cursorPos = doMoveCursor(cursorPos);
                moveCursor = false;
            }

            lastUpdateTime = currTime;

            //Serial.print("currTime ");
            //Serial.print(currTime);
            //Serial.print(" lastTimeJoyMovement ");
            //Serial.print(lastTimeJoyMovement);
            //Serial.print(" diff ");
            //Serial.println(currTime - lastTimeJoyMovement);
        }
    }

    // Print time and cursor.
    printToDisplay(
        cursorStillActive,
        cursorPos,
        time);

    return cursorStillActive;
}


/*
 * Manages display, time etc. both the standard time update, and based on user input.
 */
void display_updateDisplay(
    time_t time,
    bool &cursorActive, 
    bool &moveCursor,
    const unsigned long lastTimeJoyMovement)
{
    static cursorPosOnDisplay_t cursorPos = hourPosOnDisplay;
    unsigned long currTime = millis();
    static unsigned long lastTime_modifedNumbers = 0;
    static bool firstExec = true;
    static bool lastExecCursorActive = false;

    if (firstExec)
    {
        lastTime_modifedNumbers = millis();
        firstExec = false;
        lastExecCursorActive = false;
    } 

    cursorActive = updateDisplay(
        time,
        cursorActive,
        moveCursor,
        cursorPos,
        lastTimeJoyMovement);

    if (cursorActive && !lastExecCursorActive)
    {
        // We support hot-plug!
        display_init();
    }
    lastExecCursorActive = cursorActive;



    // Shall we allow manual updates of display?
    if (cursorActive &&
        ((currTime - lastTime_modifedNumbers) >= joystick_axis_use_read_val_delay))
    {
        switch(cursorPos)
        {
            case hourPosOnDisplay:
                if (joystick_xAxisPos())
                {
                    clock_incrementHours();
                }
                else if (joystick_xAxisNeg())
                {
                    clock_decrementHours();
                }
                break;
            case minPosOnDisplay:
                if (joystick_xAxisPos())
                {
                    clock_incrementMinutes();
                }
                else if (joystick_xAxisNeg())
                {
                    clock_decrementMinutes();
                }
                break;
            case secPosOnDisplay:
                if (joystick_xAxisPos())
                {
                    clock_incrementSeconds();
                }
                else if (joystick_xAxisNeg())
                {
                    clock_decrementSeconds();
                }
                break;
            case dayPosOnDisplay:
                if (joystick_xAxisPos())
                {
                    clock_incrementDay();
                }
                else if (joystick_xAxisNeg())
                {
                    clock_decrementDay();
                }
            break;
            default:
                // Well, we shouldn't be here.
                Serial.println("Handle user input has bad info about cursor pos!"); 
                break;
        }

        lastTime_modifedNumbers = currTime;
    }
}