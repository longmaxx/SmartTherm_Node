#ifndef DS1307_H
  #include <DS1307.h>
#endif  
#ifndef SENSORDATA_H
  #define SENSORDATA_H

#define STATE_OK 1
#define STATE_ERROR 2

class SensorData {
  public:
    byte stateTimestamp = STATE_OK;
    byte stateCelsium = STATE_OK;
    Time Timestamp;
    float Celsium;// celsium
};

#endif 
