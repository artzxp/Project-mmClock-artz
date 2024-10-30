/*
 * Common defines that you'll need to update.
 *  
 */

/* Google calendar */
#define GCALURL "https://script.google.com/macros/s/SOME-RANDOM-LONG-STRING/exec"


/* WiFi network defines  */
// Your WiFi SSID name
#define WIFI_SSID	"";
// Your WiFi SSID password
#define WIFI_PASSWD	"";
// Your clock hostname
#define WIFI_HOSTNAME	"Clock";


/* Web server defines  */
// Web server port
#define WWW_PORT	80


/* NTP defines  */
// NTP server to use
#define NTP_SERVER	162,159,200,1
// NTP port to listen on
#define NTP_PORT	123
// Time between NTP sync fetches
#define NTP_SYNC	60
// Default timezone to use
#define TIMEZONE	-4	// 
//#define TIMEZONE	-5;	// Eastern Standard Time (USA)
//#define TIMEZONE	-4;	// Eastern Daylight Time (USA)
//#define TIMEZONE	-8;	// Pacific Standard Time (USA)
//#define TIMEZONE	-7;	// Pacific Daylight Time (USA)


/* MP3 player GPIO pins */
#define SERIAL2_RXPIN	D3
#define SERIAL2_TXPIN	D2
#define BUSY_PIN	4


/* Buttons & LDR GPIO pins */
#define BUTTON1 19
#define BUTTON2 21
#define BUTTON3 22
#define LDR A0


/* FireBeetle 24x8 LED hat GPIO pins */
#if defined( ESP_PLATFORM ) || defined( ARDUINO_ARCH_FIREBEETLE8266 )  //FireBeetle-ESP32 FireBeetle-ESP8266
#define FBD_DATA D8
#define FBD_CS D9
#define FBD_WR D7
//#define FBD_RD D8
#else
#define FBD_DATA 6
#define FBD_CS 5
#define FBD_WR 7
//#define FBD_RD 8
#endif
