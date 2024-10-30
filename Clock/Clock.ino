/*
 * Time_NTP.pde
 * Example showing time sync to NTP time source
 *
 * This sketch uses the ESP8266WiFi library
 */
 
#include <TimeLib.h> 
#include <time.h> 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "DFRobot_HT1632C.h" // https://github.com/Chocho2017/FireBeetleLEDMatrix
#include <Thread.h>
#include <ThreadController.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Import my own headers.
#include "common.h"
#include "Debug.h"
#include "HTML.h"
#include "ClockNTP.h"
#include "ClockMP3.h"
//#include "GoogleCal.h"


const char ssid[] = WIFI_SSID;
const char pass[] = WIFI_PASSWD;
const char hostname[] = WIFI_HOSTNAME;

const int brightnessLevels[] = {1, 4, 8, 12, 15}; // You can adjust these levels
const int numBrightnessLevels = sizeof(brightnessLevels) / sizeof(brightnessLevels[0]);
int currentBrightnessIndex = 0;


// Set the eeprom_size to 4 
#define EEPROM_SIZE 7
// EEPROM addresses for radio button value and brightness value
#define EEPROM_RADIO 0
#define EEPROM_BRG 2
// EEPROM addresses for alarm settings
#define EEPROM_ALARM_HOUR 3
#define EEPROM_ALARM_MINUTE 4
#define EEPROM_ALARM_AMPM 5
#define EEPROM_ALARM_ENABLED 6
// Initialize EEPROM variables to store actual values when read from eeprom
uint8_t DispSavedValue;
uint8_t BRGSavedValue;
// Add new variables for alarm settings
uint8_t alarmHour = 12;
uint8_t alarmMinute = 0;
bool alarmIsPM = false;
bool alarmEnabled = false;


// Serial.print("Digit retrieved from EEPROM after 'reboot': ");
// Serial.println(DispSavedValue);
// Serial.println(SBSavedValue);
// Serial.println(BRGSavedValue);


// ThreadController that will ThreadControl all threads
ThreadController ThreadControl = ThreadController();
Thread* updateThread = new Thread();
Thread* wifiThread = new Thread();
Thread* keyThread = new Thread();
Thread* htmlThread = new Thread();
Thread* ntpThread = new Thread();
Thread* brgThread = new Thread();
Thread* gcalThread = new Thread();
Thread* alarmThread = new Thread();


WebServer server(WWW_PORT);


DFRobot_HT1632C ht1632c = DFRobot_HT1632C(FBD_DATA, FBD_WR, FBD_CS);
#define UPDATE	100		// Refresh rate of display in mS.
#define ITERATE 5		// Number of iterations before blinking.
signed int sensorLight = 0;


//bool ModeSB = 1;
int ModeBRG = 1;
int BlinkDelay = 0;
bool BlinkOn = 1;
int DispDelay = 0;
signed int ScrollPoint = 0;

bool debounceButt1 = 0;
bool debounceButt2 = 0;
bool debounceButt3 = 0;

enum enumDM
{
	dmTime24,
	dmTime12,
	dmDateYMD,
	dmDateMDY,
	dmDateFull,
	dmTDYMD24,
	dmTDYMD12,
	dmTDMDY24,
	dmTDMDY12,
	dmTDFull24,
	dmTDFull12,
	dmBlank
};
enumDM DispMode;

struct DMstruct
{
	const char *name;
	bool time24;
	bool time12;
	bool date;
	const char *format;
};

// When changing these make sure you update enumDM!
// The number of elements in both have to match.
DMstruct dm[] = {
    {"Time (24H)",          1, 0, 0,    "%H:%M"},
    {"Time (12h)",          0, 1, 0,    "%l:%M"},
    {"Date (YMD)",          0, 0, 1,    "%Y/%m/%d"},
    {"Date (MDY)",          0, 0, 1,    "%m/%d/%Y"},
    {"Date (full)",         0, 0, 1,    "%a, %d %b %Y"},
    {"Date/Time (YMD/24H)", 1, 0, 1,    "%Y/%m/%d  %H:%M:%S"},
    {"Date/Time (YMD/12h)", 0, 1, 1,    "%Y/%m/%d  %l:%M %P"},
    {"Date/Time (MDY/24H)", 1, 0, 1,    "%m/%d/%Y  %H:%M:%S"},
    {"Date/Time (MDY/12h)", 0, 1, 1,    "%m/%d/%Y  %l:%M %P"},
    {"Date/Time (full/24H)",1, 0, 1,    "%a, %d %b %Y  %H:%M:%S"},
    {"Date/Time (full/12h)",0, 1, 1,    "%a, %d %b %Y  %l:%M %P"},
    {"Display off",         0, 0, 0,    ""},
};


enum enumStatus
{
	stateWiFi,
	stateNTP,
	stateOK,
	stateAlarm,
};
enumStatus Status;

#define JSONBUFF 512		// 512byts should be enough.
#define GCALSIZE 16		// Limit to only 16 calendar entries.
struct Events {
   unsigned long event;
   const char* title;
   const char* info;
};
Events GCAL[GCALSIZE];


void keyBounce()
{
	if (Status == stateAlarm)
	{
		if (!digitalRead(BUTTON1) || !digitalRead(BUTTON1) || !digitalRead(BUTTON1))
		{
			Status = stateOK;
			UpdateDisplay();

			if (dm[DispMode].time24)
				SayTime(hour(), minute(), second(), 1);
			else if (dm[DispMode].time12)
				SayTime(hour(), minute(), second(), 0);

			if (dm[DispMode].date)
				SayDate(year(), month(), day());
		}
	}
	else
	{
		// BUTTON1 is the mode button.
		if ((digitalRead(BUTTON1) == 0) && !debounceButt1)
		{
			debounceButt1 = 1;
			CLK_PRINTLN("KEY:Button:1");

			if (dm[DispMode].time24){
        MP3_PRINTLN(String(hour()));
        MP3_PRINTLN(String(minute()));
        MP3_PRINTLN(String(second()));
				SayTime(hour(), minute(), second(), 1);
      }
			else if (dm[DispMode].time12) {
        MP3_PRINTLN(String(hour()));
        MP3_PRINTLN(String(minute()));
        MP3_PRINTLN(String(second()));
				SayTime(hour(), minute(), second(), 0);
        MP3_PRINTLN("Display mode is time12");
      }
			if (dm[DispMode].date)
      {
				SayDate(year(), month(), day());
        MP3_PRINTLN("Display mode is date");
      }
		}

		// BUTTON2 select display types.
		else if ((digitalRead(BUTTON2) == 0) && !debounceButt2)
		{
			debounceButt2 = 1;
			CLK_PRINTLN("KEY:Button:2");

			// Toggle through modes.
			int temp = DispMode;
			temp++;
			if (temp > dmBlank)
				temp = 0;
			DispMode = (enumDM)temp;
			CLK_PRINT("DM:"); CLK_PRINT(dm[DispMode].name); CLK_PRINT(":"); CLK_PRINTLN(dm[DispMode].format);

			char str[80];
			mmStrftime(str, sizeof(str), dm[DispMode].format);
			//CLK_PRINT("Now:"); CLK_PRINT(str); CLK_PRINT("  "); CLK_PRINTLN(time());
			ScrollPoint = 0;
			BlinkOn = 1;
			// handleGCAL();
			/*
			if (digitalRead(BUSY_PIN))
			{
				Play(1,1);
				// Play(1,2);
				delay(100);
			}
			// SayDate(year(), month(), day());
			*/
		}

		// BUTTON3 is refresh Google calendar
		else if ((digitalRead(BUTTON3) == 0) && !debounceButt3)
		{
			debounceButt3 = 1;
			CLK_PRINTLN("KEY:Button:3");

      // Cycle through brightness levels
      currentBrightnessIndex = (currentBrightnessIndex + 1) % numBrightnessLevels;
      ModeBRG = brightnessLevels[currentBrightnessIndex];
      ht1632c.setPwm(ModeBRG);
      UpdateDisplay();

			// handleBRG();
			// displayText("NTP");
			// handleNTP();
			// displayText("Gcal");
			// //handleGCAL();
			// displayText("Alrm");
			// handleAlarm();
			// ht1632c.inLowpower(0);
		}

		// Make sure we release before we accept a new button press.
		else if ((digitalRead(BUTTON1) == 1) && debounceButt1)
			debounceButt1 = 0;
		else if ((digitalRead(BUTTON2) == 1) && debounceButt2)
			debounceButt2 = 0;
		else if ((digitalRead(BUTTON3) == 1) && debounceButt3)
			debounceButt3 = 0;
	}
}


void connectWiFi()
{
  
  //WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    WIFI_PRINT(".");
    WIFI_PRINTLN("WiFi:Connecting.");
    //Serial.print(".");
  }

	// int Count = 10;

	// // Setup WiFi.
	// while ((WiFi.status() != WL_CONNECTED) && Count)
	// {
	// 	delay(500);
	// 	WIFI_PRINT(".");
	// 	Count--;
	// }
	// if (WiFi.status() != WL_CONNECTED)
	// {
	// 	WIFI_PRINTLN("WiFi:Connecting.");
	// 	displayText("WiFi");
	// 	WiFi.begin(ssid, pass);
	// }
}


void mmStrftime(char *str, int size, const char *format)
{
    time_t timeNow = now();
    tm *tmNow = localtime(&timeNow);
    strftime(str, size, format, tmNow);
}


void displayText(const char *Text)
{
	ht1632c.clearScreen();
	ht1632c.drawText(Text, 0, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);
	ht1632c.writeScreen();
}


void UpdateDisplay()
{
	char str[80];
	char dPM[2] = " ";
  

	ht1632c.clearScreen();
	BlinkDelay++;
	if (BlinkDelay > ITERATE)
	{
		BlinkDelay = 0;
		BlinkOn = !BlinkOn;
	}

	if (Status == stateWiFi)
	{
		if (BlinkOn)
			strcpy(str, "WiFi");
		else
			strcpy(str, "");
		// ht1632c.drawText(str, 0, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);
	}

	else if (Status == stateNTP)
	{
		if (BlinkOn)
			strcpy(str, "NTP");
		else
			strcpy(str, "");
		// ht1632c.drawText(str, 0, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);
	}

	else if (Status == stateAlarm)
	{
		if (BlinkOn)
			strcpy(str, "Alarm");
		else
			strcpy(str, "");
		// ht1632c.drawText(str, 0, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);
	}

	else
	{
		// Create time/date string.
		mmStrftime(str, 80, dm[DispMode].format);
	}

	// Remove any colons - for blinking the display.
	int index;
	for(index=0; (index<80) && str[index]; index++)
	{
		if ((str[index] == ':') && !BlinkOn)
			str[index] = ' ';
	}

	// If we have a long time/date string the scroll it.
	if (index > 5)
	{
		ht1632c.drawText(str, ScrollPoint, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);
		ScrollPoint--;
		if (ScrollPoint<-(index*4))
			ScrollPoint = 24;
		// Serial.println(ScrollPoint);
	}
	else
		ht1632c.drawText(str, 0, 0, FONT_8X4, FONT_8X4_END, FONT_8X4_HEIGHT);

	/*
		if (dPM)
		{
			// ht1632c.setPixel(x, y);
			ht1632c.drawText(dPM, 21, 0, FONT_5X4, FONT_5X4_END, FONT_5X4_HEIGHT);
		}
	*/

	ht1632c.writeScreen();
}

void htmlAlarm(String *Result, uint8_t Hour, uint8_t Minute, bool IsPM, bool Enabled)
{
    char Temp[1024];
    snprintf(Temp, sizeof(Temp), 
        alarm_html,
        Hour, Minute, 
        IsPM ? "" : " selected", IsPM ? " selected" : "",
        Enabled ? " checked" : "");
    
    Result->concat(Temp);
}


void htmlRadio(String *Result, const char *Name, int Selected, int Value, const char *Item, int savedValue)
{
    char Temp[256];

    if (Selected == Value || savedValue == Value)
    {
        snprintf(Temp, sizeof(Temp), radio_html_item, Name, Value, "checked", Item);
    }
    else
    {
        snprintf(Temp, sizeof(Temp), radio_html_item, Name, Value, "", Item);
    }

    Result->concat(Temp);
}


void htmlSlider(String *Result, char *Name, int Value, int Min, int Max, int Step)
{
  Value = BRGSavedValue;
	char Temp[256];
	sprintf(Temp, slider_html, Name, Name, Value, Min, Max, Step, Name);
	Result->concat(Temp);
}


void htmlButton(String *Result, char *Name, int Value)
{
	char Temp[256];

	if (Value)
		sprintf(Temp, button_html, Name, "checked", Name);
	else
		sprintf(Temp, button_html, Name, "", Name);

	Result->concat(Temp);
}


void handleSubmit(void)
{
    // Handle toggles.
    //ModeSB = 0;

    if (server.args() > 0)
    {
        for (uint8_t i = 0; i < server.args(); i++)
        {
            if (server.argName(i) == optDispMode)
            {
                DispMode = (enumDM) server.arg(i).toInt();
                Serial.println(server.arg(i));

                // Existing EEPROM logic (unchanged)
                // uint8_t retrievedDigit = EEPROM.read(EEPROM_ADDRESS);
                // if (retrievedDigit == 255) {
                //     Serial.println("EEPROM not initialized. Setting value.");
                // }
                EEPROM.write(EEPROM_RADIO, server.arg(i).toInt());
                EEPROM.commit();
            }
            // else if (server.argName(i) == optSB)
            // {
            //     ModeSB = 1;
            //     // Save ModeSB to EEPROM
            //     EEPROM.write(EEPROM_SB, ModeSB);
            //     EEPROM.commit();
            // }
            else if (server.argName(i) == optBRG)
            {
                ModeBRG = server.arg(i).toInt();
                ht1632c.setPwm(ModeBRG);
                if (ModeBRG)
                {
                    ht1632c.isLedOn(true);
                }
                else
                {
                    ht1632c.isLedOn(false);
                }
                // Save ModeBRG to EEPROM
                EEPROM.write(EEPROM_BRG, ModeBRG);
                EEPROM.commit();
            }

            if (server.hasArg(optAlarmHour)) {
                alarmHour = server.arg(optAlarmHour).toInt();
                EEPROM.write(EEPROM_ALARM_HOUR, alarmHour);
            }
            if (server.hasArg(optAlarmMinute)) {
                alarmMinute = server.arg(optAlarmMinute).toInt();
                EEPROM.write(EEPROM_ALARM_MINUTE, alarmMinute);
            }
            if (server.hasArg(optAlarmAmPm)) {
                alarmIsPM = (server.arg(optAlarmAmPm) == "PM");
                EEPROM.write(EEPROM_ALARM_AMPM, alarmIsPM);
            }
                alarmEnabled = server.hasArg(optAlarmEnabled);
                EEPROM.write(EEPROM_ALARM_ENABLED, alarmEnabled);

            EEPROM.commit();

            HTML_PRINT("HTML:");
            HTML_PRINT(server.argName(i));
            HTML_PRINT(" = ");
            HTML_PRINTLN(server.arg(i));
        }
    }
}


void handleRoot(void)
{
	if (server.method() == HTTP_POST)
	{
		handleSubmit();
	}

  GetEERPROMData();

	String Response;
	Response = Response + header_html;
	Response = Response + CSS_html;

	if (Status == stateNTP)
	{
		Response = Response + "<p>Status: <b>No NTP!</b></p>";
	}

	Response = Response + "<form action=\"/\" method=\"POST\">";

	Response = Response + "Display Mode:<br />";
  Response = Response + radio_html_start;
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTime24, dm[dmTime24].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTime12, dm[dmTime12].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmDateYMD, dm[dmDateYMD].name,DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmDateMDY, dm[dmDateMDY].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTDYMD24, dm[dmTDYMD24].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTDYMD12, dm[dmTDYMD12].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTDMDY24, dm[dmTDMDY24].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTDMDY12, dm[dmTDMDY12].name, DispSavedValue);
	htmlRadio(&Response, (char*)optDispMode, DispMode, dmBlank, dm[dmBlank].name, DispSavedValue);
  Response = Response + radio_html_end;

  Serial.println(Response);

  //radio_html = 	htmlRadio(&Response, (char*)optDispMode, DispMode, dmTDMDY12, dm[dmTDMDY12].name);

	Response = Response + "<br /><hr />";

	//htmlButton(&Response, (char*)optSB, ModeSB);
	htmlSlider(&Response, (char*)optBRG, ModeBRG, 0, 15, 1);

    // Add alarm settings using the new htmlAlarm function
    htmlAlarm(&Response, alarmHour, alarmMinute, alarmIsPM, alarmEnabled);

	Response = Response + "<input type = \"submit\" name = \"submit\" value = \"Submit\" /></form>";

	Response = Response + footer_html;
	server.send(200, "text/html", Response);

  //server.send(200, "text/plain", "hello from esp32!");

	// close the connection:
	// HTML_PRINTLN("Client Response:");
	// HTML_PRINTLN(Response);
}


void handleNotFound() {
  //digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}


void WiFiEvent(WiFiEvent_t event)
{
	switch(event)
	{
		case SYSTEM_EVENT_AP_START:
			// WiFi.softAPsetHostname(hostname);
			WIFI_PRINTLN("WiFi:STA AP START:");
			break;

		case SYSTEM_EVENT_STA_START:
			WiFi.setHostname(hostname);
			WIFI_PRINT("WiFi:STA START:");
			WIFI_PRINTLN(hostname);
			break;

		case SYSTEM_EVENT_STA_CONNECTED:
			WIFI_PRINT("WiFi:STA CONNECT:");
			WIFI_PRINTLN(ssid);
			break;

		case SYSTEM_EVENT_AP_STA_GOT_IP6:
			WIFI_PRINTLN("WiFi:STA AP IP6:");
			WIFI_PRINTLN(WiFi.localIPv6());
			break;

		case SYSTEM_EVENT_STA_GOT_IP:
			Status = stateNTP;
			WIFI_PRINT("WiFi:STA IP:");
			WIFI_PRINTLN(WiFi.localIP());
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			Status = stateWiFi;
			WIFI_PRINTLN("WiFi:STA DISCONNECT.");
			break;

		default:
			break;
	}
}


void handleHTML()
{
	server.handleClient();
}


void handleNTP()
{
	if (timeStatus() == timeNotSet)
	{
		NTP_PRINTLN("NTP:Lost.");
		Status = stateNTP;
		//getNtpTime();
	}
	else
		Status = stateOK;
}


void handleBRG()
{
	// The LEDs emit too much light, turn em off.
	displayText("");
	delay(500);

 //Not using the light sensor in this project so disabling the code for it.  
	// Adjust brightness
	// sensorLight = analogRead(LDR);
	// // ModeBRG = sensorLight / 256;
	// ModeBRG = map(sensorLight, 0, 3000, 16, 1);
	// if (ModeBRG < 0)
	// 	ModeBRG = 0;

	CLK_PRINT("BRG:"); CLK_PRINT(sensorLight); CLK_PRINT(" : "); CLK_PRINTLN(ModeBRG);
	ht1632c.setPwm(ModeBRG);
	ht1632c.inLowpower(0);
	UpdateDisplay();
}


void handleSay()
{
	SayTime(hour(), minute(), second(), 1);
	SayDate(year(), month(), day());
}


void handleAlarm(void)
{
    CLK_PRINTLN("Alarm check:");

    if (alarmEnabled)
    {
        char currentTime[9];  // HH:MM:SS\0
        mmStrftime(currentTime, sizeof(currentTime), "%H:%M:%S");

        int currentHour, currentMinute;
        sscanf(currentTime, "%d:%d", &currentHour, &currentMinute);

        // Convert alarm hour to 24-hour format if it's PM
        int alarmHour24 = alarmHour;
        if (alarmIsPM && alarmHour != 12) {
            alarmHour24 += 12;
        } else if (!alarmIsPM && alarmHour == 12) {
            alarmHour24 = 0;
        }

        CLK_PRINT("Current time: ");
        CLK_PRINTLN(currentTime);
        
        CLK_PRINT("Alarm set for: ");
        CLK_PRINT(alarmHour24);
        CLK_PRINT(":");
        CLK_PRINTLN(alarmMinute);

        CLK_PRINT("Alarm enabled: ");
        CLK_PRINTLN(alarmEnabled ? "Yes" : "No");

        if (currentHour == alarmHour24 && currentMinute == alarmMinute)
        {
            CLK_PRINTLN("Alarm triggered!");
            Status = stateAlarm;
            Play(1,1);
        }
        else
        {
            CLK_PRINTLN("Alarm not triggered.");
        }
    }
    else
    {
        CLK_PRINTLN("Alarm is disabled.");
    }
}


// void handleGCAL(void)
// {
// 	if (WiFi.status() == WL_CONNECTED)
// 	{
// 		String response = FetchGCal(GCALURL);
// 		GCAL_PRINT("GCAL:"); GCAL_PRINTLN(response);
// 		process(response);
// 	}
// }


void process(String response)
{
    JSON_PRINT(F("JSON:")); JSON_PRINTLN(response);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        JSON_PRINT(F("JSON:Parse error:")); JSON_PRINTLN(error.c_str());
        return;
    }

    String status = doc["status"].as<String>();
    JSON_PRINT(F("JSON:")); JSON_PRINT(status); JSON_PRINT(F(":")); JSON_PRINTLN(getNow());

    if (status == "OK") {
        if (doc.containsKey("event")) {
            JsonArray eventArray = doc["event"];
            for (int i = 0; i < GCALSIZE && i < eventArray.size(); i++) {
                GCAL[i].event = eventArray[i].as<unsigned long>();
                GCAL[i].title = doc["title"][i];
                GCAL[i].info = doc["info"][i];
                JSON_PRINT(F("EVENT: ")); JSON_PRINT(GCAL[i].event); JSON_PRINT(F(" ")); 
                JSON_PRINT(GCAL[i].title); JSON_PRINT(F(" ")); JSON_PRINTLN(GCAL[i].info);
            }
        }
    }
    else if (status == "EMPTY") {
        JSON_PRINTLN(F("JSON:EMPTY"));
    }
    else if (status == "NOK") {
        JSON_PRINT(F("JSON:")); JSON_PRINTLN(doc["error"].as<const char*>());
    }
    else {
        JSON_PRINT(F("JSON:Unknown:")); JSON_PRINTLN(response);
    }
}

void GetEERPROMData()
{
  // Retrieve the digit from EEPROM
  DispSavedValue = EEPROM.read(EEPROM_RADIO);
  BRGSavedValue = EEPROM.read(EEPROM_BRG);
  alarmHour = EEPROM.read(EEPROM_ALARM_HOUR);
  alarmMinute = EEPROM.read(EEPROM_ALARM_MINUTE);
  alarmIsPM = EEPROM.read(EEPROM_ALARM_AMPM);
  alarmEnabled = EEPROM.read(EEPROM_ALARM_ENABLED);
}


void setup() 
{
  EEPROM.begin(EEPROM_SIZE);

  GetEERPROMData();

  // Set the brightness from eeprom value
  ModeBRG = BRGSavedValue;

	//DispMode = dmTime24;
  DispMode = (enumDM)DispSavedValue;
	Status = stateWiFi;

	pinMode(BUSY_PIN, INPUT_PULLUP);
	pinMode(BUTTON1, INPUT_PULLUP);
 	pinMode(BUTTON2, INPUT_PULLUP);
 	pinMode(BUTTON3, INPUT_PULLUP);
	// pinMode(D1, OUTPUT);
	// digitalWrite(D1, 0);


	Serial.begin(115200);


	//while (!Serial) ; // Needed for Leonardo only
	delay(250);
	INIT_PRINTLN("mmClock v0.9");


	// Setup LED display.
	ht1632c.begin();
	ht1632c.isLedOn(true);
	ht1632c.setPwm(ModeBRG);


	// Setup WiFi
	INIT_PRINTLN("WiFi:");
	displayText("WiFi");
  WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
	WiFi.onEvent(WiFiEvent);


	// Setup NTP.
	INIT_PRINTLN("NTP:");
	displayText("NTP");
	SetupNTP();


	// Setup MP3 player.
	INIT_PRINTLN("MP3:");
	displayText("MP3");
	SetupMP3();


	// Setup web server.
	INIT_PRINTLN("HTML:");
	displayText("HTML");
	// server.on("/submit", handleSubmit);
  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();

	// Setup threads.
	INIT_PRINTLN("CPU:");
	displayText("CPU");
	updateThread->onRun(UpdateDisplay);	// Display update thread.
	updateThread->setInterval(UPDATE);
	wifiThread->onRun(connectWiFi);		// WiFi connection thread.
	wifiThread->setInterval(10000);
	keyThread->onRun(keyBounce);		// Key check thread.
	keyThread->setInterval(50);
	htmlThread->onRun(handleHTML);		// web server thread.
	htmlThread->setInterval(2000);
	ntpThread->onRun(handleNTP);		// NTP sync thread.
	ntpThread->setInterval(120000);
	brgThread->onRun(handleBRG);		// Regular time announcements.
	brgThread->setInterval(3600000);
	//gcalThread->onRun(handleGCAL);		// Update Google Calendar data.
	//gcalThread->setInterval(60000);
	alarmThread->onRun(handleAlarm);	// The Alarm thread.
	alarmThread->setInterval(60000);

	ThreadControl.add(updateThread);
	ThreadControl.add(wifiThread);
	ThreadControl.add(keyThread);
	ThreadControl.add(htmlThread);
	ThreadControl.add(ntpThread);
	ThreadControl.add(brgThread);
	//ThreadControl.add(gcalThread);
	ThreadControl.add(alarmThread);

	INIT_PRINTLN("OK:");
	displayText("mmC 1");
}


void loop()
{
	// Manage running threads.
	ThreadControl.run();

/*	if (!digitalRead(BUSY_PIN))
	{
		MP3_PRINTLN("U");
	}
*/
}