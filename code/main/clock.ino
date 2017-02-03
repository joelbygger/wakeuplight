


const int intComepnsatorLimit = 5;

static volatile int priv_intCounter = 0;             // counts rising edge clock signals
static volatile int priv_intCompensator = 0;         // compensates for clock inaccuracy, freq. measured to be: 490.2Hz
static time_t priv_currTime;



/**
* Upcates and compensates clock.
*
* NOTE: This function only works if no param changes > 59 ticks between calls.
*
*/
void updateClock()
{
    if (clock_incrementSeconds())
    {
        if (clock_incrementMinutes())
        {
            if (clock_incrementHours())
            {
                clock_incrementDay();
            }
        }
    }
}

/**
 * Init package global variables.
 */
void clock_init()
{
    priv_currTime.seconds = 0;
    priv_currTime.minutes = 0;
    priv_currTime.hours = 0;
    priv_currTime.dayNo = 0;
    priv_currTime.dayText = weekdays[priv_currTime.dayNo];
}


/**
* Interrupt for the clock signal.
*/
void clock_interrupt()      // called by interrupt
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
    
        updateClock();
        priv_intCounter = 0;     // Reset after 1 second is reached
    }
    
    return;
}

/*
 * General clock update functions, seconds.
 */
bool clock_incrementSeconds()
{
    bool overflow = false;
    if (priv_currTime.seconds >= 59)
    {
        priv_currTime.seconds = 0;
        overflow = true;
    }
    else
    {
        priv_currTime.seconds++;
    }
    return overflow;
}
void clock_decrementSeconds()
{
    if (priv_currTime.seconds <= 0)
    {
        priv_currTime.seconds = 59;
    }
    else
    {
        priv_currTime.seconds--;
    }
}

/*
 * General clock update functions, minutes.
 */
bool clock_incrementMinutes()
{
    bool overflow = false;
    if (priv_currTime.minutes >= 59)
    {
        priv_currTime.minutes = 0;
        overflow = true;
    }
    else
    {
        priv_currTime.minutes++;
    }
    return overflow;
}
void clock_decrementMinutes()
{
    if (priv_currTime.minutes <= 0)
    {
        priv_currTime.minutes = 59;
    }
    else
    {
        priv_currTime.minutes--;
    }
}

/*
 * General clock update functions, hours.
 */
bool clock_incrementHours()
{
    bool overflow = false;
    if (priv_currTime.hours >= 23)
    {
        priv_currTime.hours = 0;
        overflow = true;
    }
    else
    {
        priv_currTime.hours++;
    }
    return overflow;
}
void clock_decrementHours()
{
    if (priv_currTime.hours <= 0)
    {
        priv_currTime.hours = 23;
    }
    else
    {
        priv_currTime.hours--;
    }
}

/*
 * General clock update functions, day.
 */
void clock_incrementDay()
{
    if (priv_currTime.dayNo >= 6)
    {
        priv_currTime.dayNo = 0;
    }
    else
    {
        priv_currTime.dayNo++;
    }
    priv_currTime.dayText = weekdays[priv_currTime.dayNo];
}
void clock_decrementDay()
{
    if (priv_currTime.dayNo <= 0)
    {
        priv_currTime.dayNo = 6;
    }
    else
    {
        priv_currTime.dayNo--;
    }
    priv_currTime.dayText = weekdays[priv_currTime.dayNo];
}

/**
 * Returns current time,
 */
time_t clock_getTime()
{
    return priv_currTime;
}