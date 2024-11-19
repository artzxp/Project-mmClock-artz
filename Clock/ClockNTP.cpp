#include <TimeLib.h> 
#include <WiFiUdp.h>

// Import my own headers.
#include "common.h"
#include "Debug.h"

IPAddress timeServer(NTP_SERVER);
unsigned int localPort = NTP_PORT;
const int timeZone = TIMEZONE;  // Eastern Standard Time (UTC-5)

WiFiUDP Udp;

const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[NTP_PACKET_SIZE]; 

// DST calculation functions
bool isDST(time_t t) {
    tmElements_t tm;
    breakTime(t, tm);
    
    // DST starts on the second Sunday in March
    // and ends on the first Sunday in November
    if (tm.Month < 3 || tm.Month > 11) return false;  // Jan, Feb, Dec
    if (tm.Month > 3 && tm.Month < 11) return true;   // Apr-Oct
    
    // March: DST starts on second Sunday at 2:00 AM
    if (tm.Month == 3) {
        // Calculate second Sunday
        int secondSunday = 8 + (7 - ((8 + timeZone * SECS_PER_HOUR) % 7));
        return tm.Day > secondSunday || (tm.Day == secondSunday && tm.Hour >= 2);
    }
    
    // November: DST ends on first Sunday at 2:00 AM
    if (tm.Month == 11) {
        // Calculate first Sunday
        int firstSunday = 1 + (7 - ((1 + timeZone * SECS_PER_HOUR) % 7));
        return tm.Day < firstSunday || (tm.Day == firstSunday && tm.Hour < 2);
    }
    
    return false;
}

int getDSTOffset(time_t t) {
    return isDST(t) ? 3600 : 0; // Add one hour (3600 seconds) during DST
}

void sendNTPpacket(IPAddress &address)
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    
    Udp.beginPacket(address, 123);
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

time_t getNtpTime()
{
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    NTP_PRINTLN("NTP:Tx");
    sendNTPpacket(timeServer);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 3000)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            NTP_PRINTLN("NTP:Rx");
            Udp.read(packetBuffer, NTP_PACKET_SIZE);
            unsigned long secsSince1900;
            secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            
            time_t rawTime = secsSince1900 - 2208988800UL;
            // First apply timezone offset, then check if DST applies
            time_t localTime = rawTime + (timeZone * SECS_PER_HOUR);
            return localTime + getDSTOffset(localTime);
        }
    }
    NTP_PRINTLN("NTP:NACK");
    return 0;
}

void SetupNTP(void)
{
    NTP_PRINT("NTP:Listen:");
    NTP_PRINTLN(localPort);
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(NTP_SYNC);
}

void SetSyncEvent(void)
{
    //onNTPSyncEvent(TimeOK);
}

time_t getNow()
{
    return now();  // now() already includes timezone and DST adjustments
}

