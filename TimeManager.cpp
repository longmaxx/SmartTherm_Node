#include "TimeManager.h"

TimeManager::TimeManager(DS1307 &p_DS1307,WiFiUDP &p_ntpUDP, NTPClient &p_timeClient):ntpUDP(p_ntpUDP), rtc(p_DS1307), timeClient(p_timeClient)
{

}

void TimeManager::loopNTP()
{
  if (timeClient.update()){
    flag_TimeIsOK = true;
  }else{
    flag_TimeIsOK = false;
  }
}

unsigned long TimeManager::getTimestamp()
{
  return timeClient.getEpochTime();
}

