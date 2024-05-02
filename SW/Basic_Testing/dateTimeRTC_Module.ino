#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

DateTime lastTriggerTime; // Store the last trigger time

const unsigned long TRIGGER_INTERVAL = 3 * 60 * 60 * 1000; // 3 hours in milliseconds

void setup() {
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initialize last trigger time to current time
  lastTriggerTime = rtc.now();
}

void loop() {
  DateTime now = rtc.now();
  
  // Check if the trigger interval has passed
  // unixtime --> is a standard way of representing time as a single numeric value
  if (now.unixtime() - lastTriggerTime.unixtime() >= TRIGGER_INTERVAL) {
    // Trigger event here
    Serial.println("3 hours passed, triggering event!");

    // Update last trigger time
    lastTriggerTime = now;
  }

  // Compute and print the time difference
  printTimeDifference(now, lastTriggerTime);
  //printDateTime(now);

  delay(1000); // Wait for 1 second

  // Check for overflow
  if (now.unixtime() < 0) {
    Serial.println("RTC overflow detected. Resetting...");
    resetRTC();
    return; // Exit loop to reset
  }
  
}

// Function to compute and print the time difference
void printTimeDifference(DateTime currentTime, DateTime lastTrigger) {
  unsigned long diffSeconds = currentTime.unixtime() - lastTrigger.unixtime();
  unsigned long days = diffSeconds / 86400;
  unsigned long hours = (diffSeconds % 86400) / 3600;
  unsigned long minutes = (diffSeconds % 3600) / 60;
  unsigned long seconds = diffSeconds % 60;

  Serial.print("Time Difference: ");
  Serial.print(days);
  Serial.print(" days, ");
  Serial.print(hours);
  Serial.print(" hours, ");
  Serial.print(minutes);
  Serial.print(" minutes, ");
  Serial.print(seconds);
  Serial.println(" seconds");
}

// Function to print DateTime object
void printDateTime(DateTime dt) {
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(" ");
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);
  Serial.println();
}

// Function to reset the RTC module
void resetRTC() {
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
