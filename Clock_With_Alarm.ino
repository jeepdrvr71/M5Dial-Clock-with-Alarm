#include <M5Dial.h>
#include <WiFi.h>
#include <time.h>

// --- Configuration ---
const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";

// Time Server Configuration
const char* ntpServer  = "pool.ntp.org";
const long  gmtOffset_sec = -18000;    
const int   daylightOffset_sec = 3600; 

// --- Sync Timer ---
unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 86400000; 

// --- State Machine & UI ---
enum State { 
    CLOCK_VIEW, MENU_VIEW, 
    SET_ALARM_H, SET_ALARM_M, 
    SET_TIME_H, SET_TIME_M, 
    SET_DATE_M, SET_DATE_D, SET_DATE_Y,
    SYNCING_TIME, ALARM_RINGING 
};
State currentState = CLOCK_VIEW;

long oldEncoderPosition = 0;
int menuIndex = 0;
const int NUM_MENU_ITEMS = 8;
String menuItems[NUM_MENU_ITEMS] = {
    "Exit Menu", 
    "Turn Alarm ON",
    "Turn Alarm OFF", 
    "Set Alarm Time", 
    "Set Clock Time", 
    "Set Clock Date", 
    "Fetch Internet Time",
    "Toggle UI Beeps"
};

// --- Alarm & System Variables ---
bool alarmEnabled = false;
int alarmHour = 7;
int alarmMinute = 0;
bool alarmTriggeredThisMinute = false;
bool uiBeepsEnabled = true;
bool forceRedrawIcon = true; 

int tempHour = 0;
int tempMinute = 0;
int tempMonth = 1;
int tempDay = 1;
int tempYear = 2026;

// Helper to play a short click sound
void playBeep() {
    if (uiBeepsEnabled) {
        M5Dial.Speaker.tone(4000, 20); // 4000Hz frequency for 20 milliseconds
    }
}

// Helper to switch states cleanly and clear the screen
void changeState(State newState) {
    currentState = newState;
    M5Dial.Display.clear();
    oldEncoderPosition = M5Dial.Encoder.read(); 
    forceRedrawIcon = true; 
}

// Custom Wi-Fi Icon Drawing Function
void drawWiFiIcon(int x, int y, bool connected) {
    uint16_t color = connected ? TFT_GREEN : TFT_DARKGREY;
    M5Dial.Display.fillRect(x - 20, y - 18, 40, 25, TFT_BLACK);
    M5Dial.Display.drawCircle(x, y, 16, color);
    M5Dial.Display.drawCircle(x, y, 15, color); 
    M5Dial.Display.drawCircle(x, y, 9, color);
    M5Dial.Display.drawCircle(x, y, 8, color);  
    M5Dial.Display.fillRect(x - 20, y - 2, 40, 20, TFT_BLACK);
    M5Dial.Display.fillCircle(x, y + 2, 3, color);
}

// Wi-Fi NTP Sync Function
bool syncNTPTime() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5000)) {
            m5::rtc_datetime_t rtc_time;
            rtc_time.time.hours   = timeinfo.tm_hour;
            rtc_time.time.minutes = timeinfo.tm_min;
            rtc_time.time.seconds = timeinfo.tm_sec;
            rtc_time.date.year    = timeinfo.tm_year + 1900;
            rtc_time.date.month   = timeinfo.tm_mon + 1;
            rtc_time.date.date    = timeinfo.tm_mday;
            M5Dial.Rtc.setDateTime(rtc_time);
            return true;
        }
    }
    return false;
}

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK); 
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Speaker.setVolume(150); 

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    M5Dial.Display.setTextFont(&fonts::Orbitron_Light_24);
    M5Dial.Display.drawString("Syncing Time...", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    
    if (syncNTPTime()) {
        lastSyncTime = millis();
    }
    changeState(CLOCK_VIEW);
}

void loop() {
    M5Dial.update();

    long newEncoderPosition = M5Dial.Encoder.read();
    long delta = newEncoderPosition - oldEncoderPosition;
    bool buttonPressed = M5Dial.BtnA.wasPressed();
    int centerX = M5Dial.Display.width() / 2;

    // ---------------------------------------------------------
    // STATE: MAIN CLOCK VIEW
    // ---------------------------------------------------------
    if (currentState == CLOCK_VIEW) {
        if (delta != 0) {
            playBeep();
            changeState(MENU_VIEW);
            return;
        }

        if (millis() - lastSyncTime > syncInterval) {
            if (syncNTPTime()) lastSyncTime = millis();
        }

        auto t = M5Dial.Rtc.getDateTime();

        if (alarmEnabled && t.time.hours == alarmHour && t.time.minutes == alarmMinute) {
            if (!alarmTriggeredThisMinute) {
                changeState(ALARM_RINGING);
                alarmTriggeredThisMinute = true;
                return;
            }
        } else {
            alarmTriggeredThisMinute = false; 
        }

        int displayHour = t.time.hours % 12;
        if (displayHour == 0) displayHour = 12; 
        bool isPM = (t.time.hours >= 12);

        char timeString[20], dateString[20];
        snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", displayHour, t.time.minutes, t.time.seconds);
        snprintf(dateString, sizeof(dateString), "%02d/%02d/%02d", t.date.date, t.date.month, t.date.year % 100);

        M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
        M5Dial.Display.setTextPadding(180); 
        M5Dial.Display.drawString(timeString, centerX, 90);

        M5Dial.Display.setTextFont(&fonts::Orbitron_Light_24); 
        M5Dial.Display.setTextPadding(60); 
        M5Dial.Display.drawString(isPM ? "PM" : "AM", centerX, 125);

        M5Dial.Display.setTextPadding(150); 
        M5Dial.Display.drawString(dateString, centerX, 160);

        M5Dial.Display.setTextFont(&fonts::FreeSans9pt7b);
        M5Dial.Display.setTextPadding(100);
        M5Dial.Display.drawString(alarmEnabled ? "ALARM ON" : " ", centerX, 30);
        
        bool currentWiFiState = (WiFi.status() == WL_CONNECTED);
        static bool lastWiFiState = !currentWiFiState; 
        
        if (forceRedrawIcon || currentWiFiState != lastWiFiState) {
            drawWiFiIcon(centerX, 205, currentWiFiState);
            lastWiFiState = currentWiFiState;
            forceRedrawIcon = false; 
        }

        delay(50); 
    }

    // ---------------------------------------------------------
    // STATE: MENU VIEW
    // ---------------------------------------------------------
    else if (currentState == MENU_VIEW) {
        if (delta >= 2) { menuIndex++; oldEncoderPosition += 2; playBeep(); }
        if (delta <= -2) { menuIndex--; oldEncoderPosition -= 2; playBeep(); }
        
        if (menuIndex < 0) menuIndex = NUM_MENU_ITEMS - 1;
        if (menuIndex >= NUM_MENU_ITEMS) menuIndex = 0;

        M5Dial.Display.setTextFont(&fonts::FreeSans9pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString("MENU:", centerX, 80);
        
        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK); 
        M5Dial.Display.drawString(menuItems[menuIndex], centerX, 120);
        
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Dial.Display.setTextFont(&fonts::FreeSans9pt7b);
        
        // Show context sub-text depending on the option
        if (menuIndex == 1 || menuIndex == 2) {
            M5Dial.Display.drawString(alarmEnabled ? "(Currently ON)" : "(Currently OFF)", centerX, 160);
        } else if (menuIndex == 7) {
            M5Dial.Display.drawString(uiBeepsEnabled ? "(Currently ON)" : "(Currently OFF)", centerX, 160);
        } else {
            M5Dial.Display.drawString(" ", centerX, 160); 
        }

        if (buttonPressed) {
            if (uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40); 
            
            if (menuIndex == 0) changeState(CLOCK_VIEW);
            if (menuIndex == 1) { alarmEnabled = true; changeState(CLOCK_VIEW); }  // Explicitly turn ON
            if (menuIndex == 2) { alarmEnabled = false; changeState(CLOCK_VIEW); } // Explicitly turn OFF
            if (menuIndex == 3) { tempHour = alarmHour; tempMinute = alarmMinute; changeState(SET_ALARM_H); }
            if (menuIndex == 4) { 
                auto t = M5Dial.Rtc.getDateTime();
                tempHour = t.time.hours; tempMinute = t.time.minutes; changeState(SET_TIME_H); 
            }
            if (menuIndex == 5) {
                auto t = M5Dial.Rtc.getDateTime();
                tempMonth = t.date.month; tempDay = t.date.date; tempYear = t.date.year; changeState(SET_DATE_M);
            }
            if (menuIndex == 6) { changeState(SYNCING_TIME); }
            if (menuIndex == 7) { uiBeepsEnabled = !uiBeepsEnabled; changeState(CLOCK_VIEW); }
        }
    }

    // ---------------------------------------------------------
    // STATE: SETTINGS (ALARM OR TIME)
    // ---------------------------------------------------------
    else if (currentState == SET_ALARM_H || currentState == SET_TIME_H) {
        if (delta > 0) { tempHour = (tempHour + 1) % 24; oldEncoderPosition = newEncoderPosition; playBeep(); }
        if (delta < 0) { tempHour = (tempHour - 1 + 24) % 24; oldEncoderPosition = newEncoderPosition; playBeep(); }

        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString(currentState == SET_ALARM_H ? "Set Alarm Hour:" : "Set Clock Hour:", centerX, 80);
        
        char hrStr[10]; snprintf(hrStr, sizeof(hrStr), "%02d", tempHour);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Dial.Display.drawString(hrStr, centerX, 130);
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);

        if (buttonPressed) {
            if (uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40);
            changeState(currentState == SET_ALARM_H ? SET_ALARM_M : SET_TIME_M);
        }
    }

    else if (currentState == SET_ALARM_M || currentState == SET_TIME_M) {
        if (delta > 0) { tempMinute = (tempMinute + 1) % 60; oldEncoderPosition = newEncoderPosition; playBeep(); }
        if (delta < 0) { tempMinute = (tempMinute - 1 + 60) % 60; oldEncoderPosition = newEncoderPosition; playBeep(); }

        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString(currentState == SET_ALARM_M ? "Set Alarm Min:" : "Set Clock Min:", centerX, 80);
        
        char minStr[10]; snprintf(minStr, sizeof(minStr), "%02d", tempMinute);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Dial.Display.drawString(minStr, centerX, 130);
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);

        if (buttonPressed) {
            if (uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40);
            if (currentState == SET_ALARM_M) {
                alarmHour = tempHour; alarmMinute = tempMinute; alarmEnabled = true; 
            } else {
                auto t = M5Dial.Rtc.getDateTime();
                t.time.hours = tempHour; t.time.minutes = tempMinute; t.time.seconds = 0; 
                M5Dial.Rtc.setDateTime(t);
            }
            changeState(CLOCK_VIEW);
        }
    }

    // ---------------------------------------------------------
    // STATE: SETTINGS (DATE)
    // ---------------------------------------------------------
    else if (currentState == SET_DATE_M) {
        if (delta > 0) { tempMonth++; if(tempMonth > 12) tempMonth = 1; oldEncoderPosition = newEncoderPosition; playBeep(); }
        if (delta < 0) { tempMonth--; if(tempMonth < 1) tempMonth = 12; oldEncoderPosition = newEncoderPosition; playBeep(); }

        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString("Set Month:", centerX, 80);
        
        char str[10]; snprintf(str, sizeof(str), "%02d", tempMonth);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Dial.Display.drawString(str, centerX, 130);
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);

        if (buttonPressed) { if(uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40); changeState(SET_DATE_D); }
    }
    
    else if (currentState == SET_DATE_D) {
        if (delta > 0) { tempDay++; if(tempDay > 31) tempDay = 1; oldEncoderPosition = newEncoderPosition; playBeep(); }
        if (delta < 0) { tempDay--; if(tempDay < 1) tempDay = 31; oldEncoderPosition = newEncoderPosition; playBeep(); }

        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString("Set Day:", centerX, 80);
        
        char str[10]; snprintf(str, sizeof(str), "%02d", tempDay);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Dial.Display.drawString(str, centerX, 130);
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);

        if (buttonPressed) { if(uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40); changeState(SET_DATE_Y); }
    }

    else if (currentState == SET_DATE_Y) {
        if (delta > 0) { tempYear++; oldEncoderPosition = newEncoderPosition; playBeep(); }
        if (delta < 0) { tempYear--; oldEncoderPosition = newEncoderPosition; playBeep(); }

        M5Dial.Display.setTextFont(&fonts::FreeSans12pt7b);
        M5Dial.Display.setTextPadding(200);
        M5Dial.Display.drawString("Set Year:", centerX, 80);
        
        char str[10]; snprintf(str, sizeof(str), "%04d", tempYear);
        M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Dial.Display.drawString(str, centerX, 130);
        M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);

        if (buttonPressed) {
            if(uiBeepsEnabled) M5Dial.Speaker.tone(2000, 40);
            auto t = M5Dial.Rtc.getDateTime();
            t.date.month = tempMonth; t.date.date = tempDay; t.date.year = tempYear;
            M5Dial.Rtc.setDateTime(t);
            changeState(CLOCK_VIEW);
        }
    }

    // ---------------------------------------------------------
    // STATE: MANUAL INTERNET SYNC
    // ---------------------------------------------------------
    else if (currentState == SYNCING_TIME) {
        M5Dial.Display.setTextFont(&fonts::Orbitron_Light_24);
        M5Dial.Display.setTextPadding(240);
        M5Dial.Display.drawString("Syncing...", centerX, M5Dial.Display.height() / 2);
        
        if (syncNTPTime()) {
            lastSyncTime = millis();
        } else {
            M5Dial.Display.clear();
            M5Dial.Display.drawString("Sync Failed", centerX, M5Dial.Display.height() / 2);
            delay(2000);
        }
        changeState(CLOCK_VIEW);
    }

    // ---------------------------------------------------------
    // STATE: ALARM RINGING
    // ---------------------------------------------------------
    else if (currentState == ALARM_RINGING) {
        M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
        M5Dial.Display.setTextPadding(240);
        
        if (millis() % 1000 < 500) {
            M5Dial.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5Dial.Display.drawString("WAKE UP!", centerX, 120);
            M5Dial.Speaker.tone(3000, 100); 
        } else {
            M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            M5Dial.Display.drawString("WAKE UP!", centerX, 120);
        }

        if (buttonPressed) {
            M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK); 
            changeState(CLOCK_VIEW);
        }
    }
}