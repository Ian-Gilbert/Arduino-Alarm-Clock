#include <DS3232RTC.h> // clock library
#include <Keypad.h> // keypad library


// Shift Register Setup
const int clock = 49; // Clock pin on controller (pin 14 on shift register)
const int latch = 50;  // Latch pin on controller (pin 12 on shift register)
const int data = 53;   // Data pin on controller (pin 11 on shift register)


// Keypad Setup
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns

// Define the symbols on the buttons of the keypad
const char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

const byte rowPins[ROWS] = {23, 24, 27, 28}; // Row pinouts of the keypad
const byte colPins[COLS] = {31, 32, 35, 36}; // Column pinouts of the keypad

// Initialize an instance of class Keypad
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Stores the most recent keypress from the keypad.
// Used to differentiate between a short and long press.
char lastKeyPressed;


// 7-Segment Displays Setup
const int digits[] = {10, 8, 6, 4, 2}; // Pins digits[0]-[3] control digits 1-4 (left to right) on 4-dgit display, digits[4] controls single digit display

const unsigned char noPoints[] = { // Hex code for digits 0-9, blank, A, and P
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f,
    0x00, 0x77, 0x73
};
const unsigned char points[] = { // Same as above but with the point displayed
    0xbf, 0x86, 0xdb, 0xcf, 0xe6,
    0xed, 0xfd, 0x87, 0xff, 0xef,
    0x80, 0xf7, 0xf3
};

const int AM = 11; // index of 'A' in both arrays
const int PM = 12; // index of 'P' in both arrays
const int BLANK = 10; // index of 'blank' in both arrays


time_t alarmTime = 0; // Temporary until I can figure out how to correctly
time_t snoozeTime;    //    read the alarm time from the ds3231 registers.


// Buzzer Setup
const int buzzer = 12; // pin controlling active buzzer


// Global flags
boolean isAlarmRunning = false; // if alarm is currently activated
boolean isAlarmOn = false; // if the alarm is turned on and will sound once triggered
boolean isSnoozeOn = false; // same for the snooze
boolean is12HourMode = true; // 12 or 24 hour time (default 12)


void setup() {
    // set pins for shift register:
    pinMode(latch, OUTPUT);
    pinMode(clock, OUTPUT);
    pinMode(data, OUTPUT);

    // set pins for both displays (digits[4] is the 1-digit display,
    // digit[0] is the first digit on the 4-digit display, and so on):
    for (int i = 0; i <= 4; i++) {
        pinMode(digits[i], OUTPUT);
    }

    // set pin for alarm:
    pinMode(buzzer, OUTPUT);

    // Set the amount of milliseconds before the keypad enters the HOLD state.
    keypad.setHoldTime(2000);

    // Initialize the alarms to known values and clear the alarm flags
    RTC.setAlarm(ALM1_MATCH_HOURS, 0, 0, 0);
    RTC.setAlarm(ALM2_MATCH_HOURS, 0, 0, 0);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);

    setSyncProvider(RTC.get);
}


/* Releases a digit with no point through the shift register */
void displayNoPoint(int num) {
    digitalWrite(latch, LOW);
    shiftOut(data, clock, MSBFIRST, noPoints[num]);
    digitalWrite(latch, HIGH);
}


/* Releases a digit with a point through the shift register */
void displayPoint(int num) {
    digitalWrite(latch, LOW);
    shiftOut(data, clock, MSBFIRST, points[num]);
    digitalWrite(latch, HIGH);
}


/* Toggles a digit on the 7-segment displays on and off once.
 * i (0-4) determines which digit is toggled. */
void toggleDigit(int i) {
    digitalWrite(digits[i], LOW); // digit on
    delay(1); // display the digit for 1 ms
    digitalWrite(digits[i], HIGH); // digit off
}


/* Convert from hour, minute, am/pm (12 hour mode) to time_t (total seconds).
 * Essentially, converts to 24 hour time, then converts hours and minutes to total seconds. */
time_t convertToTime_T(int hour, int minute, boolean isPm) {
    time_t t = 0;
    if (hour == 12) {
        if (isPm) t += hour * 3600L; // 12 PM
    } // 12 AM is hour 0, so add nothing to t
    else if (isPm) t += (hour + 12L) * 3600L; // 1-11 PM add 12 hours
    else t += hour * 3600L; // 1-11 AM
    t += minute * 60L;
    return t;
}


/* Convert from hour and minute (24 hour mode) to time_t. */
time_t convertToTime_T(int hour, int minute) {
    time_t t = hour * 3600L;
    t += minute * 60L;
    return t;
}


/* Display the time given by t. */
void displayTime(time_t t) {
    if (is12HourMode) {
        displayAmPm(isPM(t));
        displayHour(hourFormat12(t));
    } else { // 24 hour time
        displayBlankLastDigit(); // no AM/PM, but we might want to display alarm on
        displayHour(hour(t));
    }
    displayMinute(minute(t));
}


/* Displays the hour. Sets the first digit, then turns it on and off.
 * Then does the same with the second digit. */
void displayHour(int hour) {
    // Split the hour into the first and second digit
    int digitOne = hour / 10;
    int digitTwo = hour % 10;

    if (digitOne == 0) { // If the hour is one digit, leave the first digit blank (3:00 instead of 03:00, for example).
        // Displays nothing, but if I don't call it, it doesn't work right.
        // Haven't figure that one out yet.
        displayNoPoint(BLANK);
    } else {
        displayNoPoint(digitOne);
    }
    toggleDigit(0);
    displayPoint(digitTwo); // add point between hour and minute
    toggleDigit(1);
}


/* Displays the minute. Same process as displayHour(), except we never have to display blank. */
void displayMinute(int minute) {
    int digitThree = minute / 10;
    int digitFour = minute % 10;
    
    displayNoPoint(digitThree);
    toggleDigit(2);
    displayNoPoint(digitFour);
    toggleDigit(3);
}


/* Displays AM/PM - adds point if alarm is on. */
void displayAmPm(boolean isPm) {
    if (isAlarmOn) {
        if (isPm) displayPoint(PM);
        else displayPoint(AM);
    } else {
        if (isPm) displayNoPoint(PM);
        else displayNoPoint(AM);
    }
    toggleDigit(4);
}


/* If alarm is on, display a point for the last digit, otherwise do nothing (24 hour mode). */
void displayBlankLastDigit() {
    if (isAlarmOn) {
        displayPoint(BLANK);
    } else {
        displayNoPoint(BLANK);
    }
    toggleDigit(4);
}


/* Read the alarm time from the ds3231 registers. alarmNumber determines alarm 1 or 2.
 * DOES NOT WORK: readRTC calls are not returning the correct thing. */
//time_t getAlarmTime(int alarmNumber) {
//    int hour;
//    int minute;
//    if (alarmNumber == ALARM_1) {
//        hour = RTC.readRTC(0x09); // Alarm 1 hours register
//        minute = RTC.readRTC(0x08); // Alarm 1 minutes register
//    } else {
//        hour = RTC.readRTC(0x0C); // Alarm 2 hours register
//        minute = RTC.readRTC(0x0B); // Alarm 2 minutes register
//    }
//    return convertToTime_T(hour, minute);
//}


/* Displays the time the alarm is currently set to. */
void displayAlarmTime() {
//    time_t t = getAlarmTime(ALARM_1);
    for (int i = 0; i < 200; i++) { // Repeat 200 times so time displays for ~1 sec.
//        displayTime(t);
        displayTime(alarmTime); // Temporary fix until I can get getAlarmTime() working
    }
}


void setSnooze() {
    time_t t;
//    if (isSnoozeOn) t = getAlarmTime(ALARM_2);
//    else t = getAlarmTime(ALARM_1);
    if (isSnoozeOn) t = snoozeTime;
    else t = alarmTime;
    t += (10L * 60L); // add 10 minutes to time
    snoozeTime = t; // temporary until getAlarmTime() works
    RTC.setAlarm(ALM2_MATCH_HOURS, minute(t), hour(t), 0); // set alarm 2 to snooze alarm
    RTC.alarm(ALARM_2); // clear flags
}


/*  Set the clock time. */
void setClockTime() {
    time_t t = userSetTime(); // Get the time from the user input
    RTC.set(t);
    confirmBeeps(1); // actually two beeps because of the residual beep from setting the last digit
}


/* Set the alarm time. */
void setAlarmTime() {
    time_t t = userSetTime();
    alarmTime = t; // temporary until getAlarmTime() works
    RTC.setAlarm(ALM1_MATCH_HOURS, minute(t), hour(t), 0);
    RTC.alarm(ALARM_1); // clear flag
    confirmBeeps(1); // actually two beeps because of the residual beep from setting the last digit
    displayAlarmTime();
}


/* Get a time from the user and return it as time_t. */
time_t userSetTime() {
    int timeDigits[] = {BLANK, BLANK, BLANK, BLANK, BLANK}; // set each of the five digits individually
    for (int digit=0; digit < (is12HourMode ? 5 : 4); digit++) { // skip am/pm if in 24 hour mode
        setDigit(digit, timeDigits); // once a user selects a digit, store it in timeDigits
        confirmBeeps(1);
    }

    // Convert timeDigits to hour, minute, am/pm (if 12 hour mode). Return as time_t.
    if (is12HourMode) {
        int hour = (timeDigits[0] * 10) + timeDigits[1];
        int minute = (timeDigits[2] * 10) + timeDigits[3];
        boolean isPm = (timeDigits[4] == PM) ? true : false;
        return convertToTime_T(hour, minute, isPm);
    } else {
        int hour = (timeDigits[0] * 10) + timeDigits[1];
        int minute = (timeDigits[2] * 10) + timeDigits[3];
        return convertToTime_T(hour, minute);
    }
}


/* Display the time that's been set so far, with a blinking dot on the digit currently being set.
 * Once the user selects a digit, store it in timeDigits. */
void setDigit(int digit, int *timeDigits) {
    char currentKey = NULL; // will eventually be the digit to set
    unsigned int count = 0;
    while (!currentKey) { // until user selects valid key
        for (int i=0; i<5; i++) {
            // display partial time while waiting for user input
            if (i == digit) { // current digit being set
                if (count < 50) { // display flashing dot on digit currently being set
                    displayPoint(BLANK);
                    toggleDigit(digit);
                }
            }
            else if (i == 1) displayPoint(timeDigits[1]); // add dot between hour and minute
            else displayNoPoint(timeDigits[i]); // if digit hasn't been set, will display blank
            toggleDigit(i);
        }
        count = (count + 1) % 100; // loops around 0-100

        char nextKey = keypad.getKey(); // next key pressed on keypad
        if (isValidKey(nextKey, digit, timeDigits[0]))
            currentKey = nextKey;
    }

    // If we're setting AM/PM, we want to store the AM or PM constants, rather that the actual char values.
    // Otherwise, convert from char to int for the digits.
    if (digit == 4) {
        if (currentKey == '*') // '*' = AM
            timeDigits[4] = AM;
        else timeDigits[4] = PM; // '#' = PM
    } else timeDigits[digit] = currentKey - '0'; // subtract the ascii values to get the int
}


/* Check if the key entered by the user is valid for the digit they are trying to set.
 * So for example, the first digit of the minute can only be 0-5, while the second can be 0-9. */
boolean isValidKey(char nextKey, int digit, int firstHourDigit) {
    switch (digit) {
        case 0: // first hour digit
            if (nextKey == '0' || nextKey == '1')
                return true;
            else if (!is12HourMode && nextKey == '2')
                return true;
            else return false;
            break;
            
        case 1: // second hour digit
            switch(firstHourDigit) {
                case 0:
                    if (nextKey >= 49 && nextKey <= 57) // 1-9 char values (hour 1-9)
                        return true;
                    else if (!is12HourMode && nextKey == '0') return true; // allow hour=0 in 24 hour mode
                    else return false;
                    break;
                case 1:
                    if (is12HourMode) {
                        if (nextKey >= 48 && nextKey <= 50) // 0-2 char values (hour 10-12) in 12 hour mode
                            return true;
                        else return false;
                    } else {
                        if (nextKey >= 48 && nextKey <= 57) // 0-9 char values (hour 10-19) in 24 hour mode
                            return true;
                        else return false;
                    }
                    break;
                case 2: // should be guarenteed 24 hour mode
                    if (nextKey >= 48 && nextKey <= 51) // 0-3 char values (hour 20-23) in 24 hour mode
                        return true;
                    else return false;
                    break;
                default: // should never happen
                    return false;
                    break;
            }
            break;
            
        case 2: // first minute digit
            if (nextKey >= 48 && nextKey <= 53) // 0-5 char values
                return true;
            else return false;
            break;
            
        case 3: // second minute digit
            if (nextKey >= 48 && nextKey <= 57) // 0-9 char values
                return true;
            else return false;
            break;
            
        case 4: // am/pm digit
            if (nextKey == '*' || nextKey == '#') // AM or PM, respectively
                return true;
            else return false;
            break;
            
        default:
            return false;
            break;
    }
}


/* Quick beeps to confirm an action. Number of beeps specified by numBeeps.
 * Buzzer is on and off for 50 ms each. */
void confirmBeeps(int numBeeps) {
    for (int i = 0; i < numBeeps; i++) {
        digitalWrite(buzzer, HIGH); // buzzer on
        delay(50);
        digitalWrite(buzzer, LOW); // buzzer off
        delay(50);
    }
}


// Sound the alarm. While the buzzer is on/off, display the time.
void soundAlarm(time_t t) {
    Serial.println("here");
    for (int i = 0; i < 4; i++) { // causes groups of 4 beeps for the alarm
        decodeKeypad(); // if user stops alarm during beeps
        digitalWrite(buzzer, HIGH); // buzzer on
        for (int i = 0; i < 10; i++) { // approx. 50 ms delay for the buzzer
            displayTime(t);
        }
        digitalWrite(buzzer, LOW); // buzzer off
        for (int i = 0; i < 10; i++) { // approx. 50 ms delay for the buzzer
            displayTime(t);
        }
    }
}


/* Check if user has entered a command on the keypad. Take action if neccessary.
 * Returns a 1 if an action was taken, 0 otherwise. */
int decodeKeypad() {
    char currentKey = keypad.getKey();
    KeyState state = keypad.getState();

    if (currentKey) {
        lastKeyPressed = currentKey; // if user pressed a new key, update lastKeyPressed
    }

    switch (lastKeyPressed) {
        case 'A': // Snooze
            if (isAlarmRunning) {
                setSnooze();
                isAlarmRunning = false;
                isSnoozeOn = true;
            }
            lastKeyPressed = NULL;
            return 1;
        case 'B': // Alarm on/off
            if (isAlarmRunning) {
                isAlarmRunning = false;
                isSnoozeOn = false;
            }
            else if (isSnoozeOn) {
                confirmBeeps(1);
                isSnoozeOn = false;
            }
            else isAlarmOn = !isAlarmOn;
            lastKeyPressed = NULL;
            return 1;
        case 'C':
            if (state == RELEASED) { // 12/24 hour format
                is12HourMode = !is12HourMode;
                lastKeyPressed = NULL;
                return 1;
            }
            else if (state == HOLD) { // set time
                confirmBeeps(1);
                setClockTime();
                lastKeyPressed = NULL;
                return 1;
            }
            else return 0;
        case 'D':
            if (state == RELEASED) { // view alarm time
                displayAlarmTime();
                lastKeyPressed = NULL;
                return 1;
            }
            else if (state == HOLD) { // set alarm
                confirmBeeps(1);
                setAlarmTime();
                isSnoozeOn = false;
                lastKeyPressed = NULL;
                return 1;
            }
            else return 0;
        default: // no key pressed or key that does nothing
            lastKeyPressed = NULL;
            return 0;
    }
}


void loop() {
    if (RTC.alarm(ALARM_1) && isAlarmOn) isAlarmRunning = true; // should we trigger the alarm?
    else if (RTC.alarm(ALARM_2) && isSnoozeOn) isAlarmRunning = true; // should we trigger the snooze alarm?

    time_t t = RTC.get(); // refresh the time from the DS3231
    
    if (isAlarmRunning) soundAlarm(t);

    // We don't need to constantly refresh the time from the DS3231.
    // This should come out to about 2 refrehes per second.
    for (int i=0; i<100; i++) {
        displayTime(t);
        int flag = decodeKeypad();
        if (flag) break;
    }
}
