# Arduino Alarm Clock
<style>
.rotate270 {
    transform: rotate(270deg);
}
</style>

<p align="center">
	<img src="pictures/IMG_0099.png" width=50% class="rotate270"></img>
</p>

This was a project that I started for a physics class in college, but I've since updated things. The goal was to create a project using the [Elegoo Most Complete Starter Kit](https://www.elegoo.com/product/elegoo-mega-2560-project-the-most-complete-starter-kit/) (originally we used the UNO, but I upgraded to the MEGA 2560 for the extra ports). The alarm clock has 12/24 hour time modes, and an alarm that can be set, viewed, turned on/off, stopped, and snoozed.

## Hardware
- Arduino MEGA 2560
  - Note: if you have an UNO, you won't be able to fit the keypad module, so you'll have to find another way to control the clock. My original design in college used the IR remote, which worked but wasn't particularly nice. I think the better option would be to use the standard buttons, which is something I'm planning on adding in a future update.
- 74HC595 Shift Register
- 1 and 4 digit 7-segment display (except actually 8-segment display cause of the dot)
- DS3231 Real Time Clock Module
- Active Buzzer
- Keypad Module

## Libraries
- [DS3232RTC](https://www.arduinolibraries.info/libraries/ds3232-rtc)
- [Kaypad](https://www.arduinolibraries.info/libraries/keypad)

## Things To Come
When I originally did the project in school, I wrote a couple technical papers describing how the clock works, but with all the changes I've made since then, it's very out of date. I will be fixing that and adding it to the project shortly, along with instructions on how to set it all up.

Further down the line, I've got other ideas. I'm not thrilled with how the keypad turned out, from a UX perspective, so I'm considering replacing it with standard buttons, kind of like your average alarm clock. This might also make the project a little more accessible to people with an Arduino UNO.

### Known Issues
- When snooze is on, the alarm on light should flash to distinguish it from the normal alarm. It doesn't because I haven't told it to yet. This will change.
- I haven't figured out how to read the current alarm time from the DS3231 registers. I implemented a temporary fix so everything still works, but I would like to solve this.
- The brightness of the seven-segment displays is a little inconsistent. Not a huge deal, but fixing this would be nice. Not entirely sure it's possible though.
