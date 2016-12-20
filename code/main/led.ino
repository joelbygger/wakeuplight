


typedef enum
{
    alarmState_inactive,
    alarmState_active
}alarmState_t;

const uint32_t ALARM_START_TIME_HOUR = 06;
const uint32_t ALARM_START_TIME_MIN = 15;

const unsigned int ledLinearity[31] = {
    0, 1, 2, 3, 4, 6, 8, 10, 13, 16, 
    20, 24, 29, 34, 40, 47, 54, 63, 72, 82, 
    92, 104, 116, 130, 145, 160, 177, 195, 214, 234, 
    255
};



/**
 * Turns LED on and off.
 */
void led_controlLED(const time_t time)
{ 
    static uint32_t lastAlarmMinute = 99; // Something more than a legal minute.
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