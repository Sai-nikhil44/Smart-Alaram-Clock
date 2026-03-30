#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// ---------------- BUTTONS ----------------
#define BTN_MODE   32
#define BTN_ADJUST 33
#define BTN_SELECT 27
#define BTN_ALARM  26

// ---------------- BUZZER ----------------
#define BUZZER 25

// ---------------- VARIABLES ----------------
int screen = 0; 
// 0 = Main, 1 = Time Set, 2 = Alarm Set

bool editMode = false;
int editField = 0;

// Time set variables
int setHour, setMinute, setDay, setMonth, setYear;

// ---------------- ALARMS ----------------
struct Alarm {
  int hour;
  int minute;
  bool enabled;
};

#define TOTAL_ALARMS 3
Alarm alarms[TOTAL_ALARMS];

int currentAlarm = 0;

// ---------------- BUTTON STATES ----------------
bool lastModeState = HIGH;
bool lastSelectState = HIGH;
bool lastAdjustState = HIGH;
bool lastAlarmState = HIGH;

// Blink
unsigned long lastBlink = 0;
bool blinkState = true;

// Alarm ringing
bool alarmRinging = false;

// ---------------- SETUP ----------------
void setup() {
  Wire.begin(21, 22);
  rtc.begin();

  lcd.init();
  lcd.backlight();

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_ADJUST, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_ALARM, INPUT_PULLUP);

  pinMode(BUZZER, OUTPUT);

  // Initialize alarms
  for (int i = 0; i < TOTAL_ALARMS; i++) {
    alarms[i].hour = 6;
    alarms[i].minute = 0;
    alarms[i].enabled = false;
  }
}

// ---------------- LOOP ----------------
void loop() {
  DateTime now = rtc.now();

  handleButtons();

  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  checkAlarms(now);

  if (screen == 0) displayMain(now);
  else if (screen == 1) displayTimeSet();
  else displayAlarmSet();

  delay(50);
}

// ---------------- MAIN DISPLAY ----------------
void displayMain(DateTime now) {
  lcd.clear();

  lcd.setCursor(0, 0);
  print2(now.hour()); lcd.print(":");
  print2(now.minute()); lcd.print(":");
  print2(now.second());

  lcd.setCursor(0, 1);

  print2(now.day()); lcd.print("/");
  print2(now.month());
}

// ---------------- TIME SET ----------------
void displayTimeSet() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(editMode ? "EDIT TIME" : "SET TIME");

  lcd.setCursor(0, 1);

  if (editMode && editField == 0 && !blinkState) lcd.print("  ");
  else print2(setHour);

  lcd.print(":");

  if (editMode && editField == 1 && !blinkState) lcd.print("  ");
  else print2(setMinute);

  lcd.print(" ");

  if (editMode && editField == 2 && !blinkState) lcd.print("  ");
  else print2(setDay);

  lcd.print("/");

  if (editMode && editField == 3 && !blinkState) lcd.print("  ");
  else print2(setMonth);

  lcd.print("/");

  if (editMode && editField == 4 && !blinkState) lcd.print("    ");
  else lcd.print(setYear);
}

// ---------------- ALARM DISPLAY ----------------
void displayAlarmSet() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(editMode ? "EDIT AL " : "ALARM ");
  lcd.print(currentAlarm + 1);

  lcd.setCursor(0, 1);

  Alarm a = alarms[currentAlarm];

  if (editMode && editField == 0 && !blinkState) lcd.print("  ");
  else print2(a.hour);

  lcd.print(":");

  if (editMode && editField == 1 && !blinkState) lcd.print("  ");
  else print2(a.minute);

  lcd.print(" ");

  if (editMode && editField == 2 && !blinkState) {
    lcd.print("   ");
  } else {
    lcd.print(a.enabled ? "ON" : "OFF");
  }
}

// ---------------- BUTTON HANDLING ----------------
void handleButtons() {

  bool modeState = digitalRead(BTN_MODE);
  bool selectState = digitalRead(BTN_SELECT);
  bool adjustState = digitalRead(BTN_ADJUST);
  bool alarmState = digitalRead(BTN_ALARM);

  // MODE
  if (modeState == LOW && lastModeState == HIGH) {
    screen = (screen + 1) % 3;
    editMode = false;

    if (screen == 1) {
      DateTime now = rtc.now();
      setHour = now.hour();
      setMinute = now.minute();
      setDay = now.day();
      setMonth = now.month();
      setYear = now.year();
      editField = 0;
    }
  }

  // SELECT
  if (selectState == LOW && lastSelectState == HIGH) {
    if (editMode) {
      if (screen == 1) editField = (editField + 1) % 5;
      if (screen == 2) editField = (editField + 1) % 3;
    } else {
      if (screen == 2) currentAlarm = (currentAlarm + 1) % TOTAL_ALARMS;
    }
  }

  // ADJUST
  if (adjustState == LOW && lastAdjustState == HIGH && editMode) {

    if (screen == 1) {
      if (editField == 0) setHour = (setHour + 1) % 24;
      if (editField == 1) setMinute = (setMinute + 1) % 60;
      if (editField == 2) setDay = (setDay % 31) + 1;
      if (editField == 3) setMonth = (setMonth % 12) + 1;
      if (editField == 4) setYear++;
    }

    if (screen == 2) {
      if (editField == 0)
        alarms[currentAlarm].hour = (alarms[currentAlarm].hour + 1) % 24;

      else if (editField == 1)
        alarms[currentAlarm].minute = (alarms[currentAlarm].minute + 1) % 60;

      else if (editField == 2)
        alarms[currentAlarm].enabled = !alarms[currentAlarm].enabled;
    }
  }

  // ALARM BUTTON
  if (alarmState == LOW && lastAlarmState == HIGH) {

    // Stop buzzer
    if (alarmRinging) {
      alarmRinging = false;
      digitalWrite(BUZZER, LOW);
      return;
    }

    if (screen == 1) {
      editMode = !editMode;

      if (!editMode) {
        rtc.adjust(DateTime(setYear, setMonth, setDay, setHour, setMinute, 0));
      }
    }

    if (screen == 2) {
      editMode = !editMode;
      editField = 0;
    }
  }

  lastModeState = modeState;
  lastSelectState = selectState;
  lastAdjustState = adjustState;
  lastAlarmState = alarmState;
}

// ---------------- ALARM CHECK ----------------
void checkAlarms(DateTime now) {

  for (int i = 0; i < TOTAL_ALARMS; i++) {
    if (alarms[i].enabled &&
        now.hour() == alarms[i].hour &&
        now.minute() == alarms[i].minute &&
        now.second() == 0) {

      alarmRinging = true;
    }
  }

  if (alarmRinging) digitalWrite(BUZZER, HIGH);
}

// ---------------- HELPER ----------------
void print2(int num) {
  if (num < 10) lcd.print("0");
  lcd.print(num);
}