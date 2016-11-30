


const int intComepnsatorLimit = 5;

static volatile int priv_intCounter = 0;             // counts rising edge clock signals
static volatile int priv_intCompensator = 0;         // compensates for clock inaccuracy, freq. measured to be: 490.2Hz
static time_t priv_currTime;

/**
 * Init package global variables.
 */
void clock_init()
{
    priv_currTime.seconds = 0;
    priv_currTime.minutes = 0;
    priv_currTime.hours = 0;
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
    
        priv_currTime.seconds ++; // Don't update in general function, update here, faster and will work good enough.
        priv_intCounter = 0;     // Reset after 1 second is reached
    }
    
    return;
}

/*
 * General clock update functions.
 */
void clock_incrementSeconds()
{
    priv_currTime.seconds++;
}
void clock_decrementSeconds()
{
    priv_currTime.seconds--;
}

void clock_incrementMinutes()
{
    priv_currTime.minutes++;
}
void clock_decrementMinutes()
{
    priv_currTime.minutes--;
}

void clock_incrementHours()
{
    priv_currTime.hours++;
}
void clock_decrementHours()
{
    priv_currTime.hours--;
}

/**
* Upcates and compaensates clock.
*
* NOTE: This function only works if no param changes > 59 ticks between calls.
*
*/
void clock_updateClock()
{
    if (priv_currTime.seconds >= 60)
    {
        clock_incrementMinutes();
        priv_currTime.seconds = 0;
    }
    if(priv_currTime.minutes >= 60)
    {
        clock_incrementHours();
        priv_currTime.minutes = 0;
    }
    if(priv_currTime.hours >= 24)
    {
        priv_currTime.hours = 0;
    }
}

/**
 * Returns current time,
 */
time_t clock_getTime()
{
    return priv_currTime;
}