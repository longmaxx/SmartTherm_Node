#ifndef DS1307_H
  #include <DS1307.h>
#endif  
#ifndef SENSORDATA_H
  #define SENSORDATA_H

#define STATE_OK 1
#define STATE_ERROR 2

struct SensorData {
  public:
    byte stateTimestamp = STATE_OK;
    byte stateCelsium = STATE_OK;
    byte stateHumidity = STATE_ERROR;
    Time Timestamp;
    float Celsium;// celsium
    int Humidity;
};

#endif 
