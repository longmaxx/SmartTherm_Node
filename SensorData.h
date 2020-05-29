//#ifndef DS1307_H
  //#include <DS1307.h>
//#endif  
#ifndef SENSORDATA_H
  #define SENSORDATA_H

#define STATE_OK 0
#define STATE_ERROR 2

#define MAX_DS18B20_COUNT (10)

typedef struct {
  DeviceAddress address;
  int celsium;
} ds18b20_Result;

typedef struct {
  public:
    byte stateTimestamp = STATE_OK;
    unsigned long Timestamp;
    
    byte stateDHT = STATE_OK;
    float DHTCelsium;// celsium
    float DHTHumidity;
} DHTSensorData;

ds18b20_Result DS18B20_Values[MAX_DS18B20_COUNT];
unsigned char  DS18B20_count = 0;

#endif 
