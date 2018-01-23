#ifndef TIMEMANAGER_H
  #define TIMEMANAGER_H
#include <Arduino.h>
#include <NTPClient.h>
#include <DS1307.h>
#include <WiFiUdp.h>

class TimeManager{
  public:
    TimeManager(DS1307 &p_DS1307,WiFiUDP &p_ntpUDP, NTPClient &p_timeClient);
    unsigned long getTimestamp();
    void loopNTP();
    bool flag_TimeIsOK;
  private:
    WiFiUDP &ntpUDP;
    DS1307 &rtc;
    NTPClient & timeClient;
    //void initDS1307();
    //int setDS1307DateTime();
    //int getDS1307DateTime(&Time t);
    void initNTP();
};


#endif  
