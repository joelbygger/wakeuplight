


typedef enum
{
    alarmState_inactive,
    alarmState_active
}alarmState_t;

const uint32_t ALARM_START_TIME_HOUR = 6;
const uint32_t ALARM_START_TIME_MIN = 0;
// The alarm minutes counter does not wrap at 60, so this digit can be bigger.
const uint32_t MINUTES_WITH_FULL_LIGHT = 20;
// The alarm minutes counter does not wrap at 60, so this array can be bigger.
const unsigned int ledLinearity[31] = {
    0, 1, 2, 3, 4, 6, 8, 10, 13, 16, 
    20, 24, 29, 34, 40, 47, 54, 63, 72, 82, 
    92, 104, 116, 130, 145, 160, 177, 195, 214, 234, 
    255
};
const uint32_t LED_FADE_TIME = sizeof(ledLinearity)/sizeof(ledLinearity[0]);



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
                (time.minutes == ALARM_START_TIME_MIN) &&
                time.dayNo <= 4) // Only alarm on weekdays.
            {
                Serial.print("Alarm started at time: ");
                Serial.println(time.seconds);
                alarmMinute = 0;
                int pwmVal_ = ledLinearity[alarmMinute];
                Serial.println(pwmVal_);
                analogWrite(ledPin, pwmVal_);
                alarmMinute++;
                lastAlarmMinute = time.minutes;

                alarmState = alarmState_active;
            }
            break;

        case alarmState_active:
        {
            if (lastAlarmMinute != time.minutes)
            {
                // Shall we turn off?
                if ( alarmMinute >= LED_FADE_TIME )
                {
                    if (alarmMinute >= LED_FADE_TIME + MINUTES_WITH_FULL_LIGHT)
                    {
                        Serial.print("Alarm stop at time: ");
                        Serial.print(time.seconds);
                        alarmMinute = 0;

                        analogWrite(ledPin, 0);
                        alarmState = alarmState_inactive;
                    }
                    else
                    {
                        lastAlarmMinute = time.minutes;
                        alarmMinute++;
                    }
                }
                else
                {
                    Serial.print("Alarm update at time: ");
                    Serial.print(time.seconds);
                    Serial.print(" new alarm minute: ");
                    Serial.print(alarmMinute);
                    Serial.print("\n");
                    int pwmVal_ = ledLinearity[alarmMinute];
                    Serial.print(pwmVal_);
                    Serial.print("\n");
                    analogWrite(ledPin, pwmVal_);

                    lastAlarmMinute = time.minutes;
                    lastAlarmMinute = time.seconds;
                    alarmMinute++;
                }
            }
            break;
        }

        default:
            alarmState = alarmState_inactive;
            break;
    }
}