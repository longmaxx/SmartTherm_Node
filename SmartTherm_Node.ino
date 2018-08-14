#include <DallasTemperature.h>
#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "SensorData.h"
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
//#include <NTPClient.h>
#include <PubSubClient.h>
#include <Ticker.h>

bool flagSleep = true;
Ticker ticker;// task timer

String HostIP = "192.168.1.110:80";
String Url = "/web/index.php?r=temperatures/commit";
String DeviceName = "nano";

String wifiName = "KotNet";
String wifiPwd  = "MyKotNet123";

const char *mqtt_server = "192.168.1.52";//"m14.cloudmqtt.com"; // Имя сервера MQTT
const int mqtt_port = 1883;//15303; // Порт для подключения к серверу MQTT
const char *mqtt_user = "";//"aNano1"; // Логи от сервер
const char *mqtt_pass = "";//"Hui123"; // Пароль от сервера

String sCelsiumTopic = DeviceName + "/Celsium";
String sHumidityTopic = DeviceName + "/Humidity";

SimpleDHT22 Dht;
ESP8266WiFiMulti WiFiMulti;
WiFiClient wclient;
/*
4 (D2)  - RTC
5 (D1)  - RTC 
12(D6) - 1-Wire (DS18B20 or DHT22)
16(D0) connect to RST for SLEEP WAKE UP work

*/
#define PIN_DHT 12
#define PIN_1WIRE 14
#define DBG_PORT Serial



OneWire Wire1Port(PIN_1WIRE);
DallasTemperature DT(&Wire1Port);
#define MAX_DS18B20_COUNT (10)
DeviceAddress pAddr_DS18B20[MAX_DS18B20_COUNT];
int ds18b20_count = 0;
  
DeviceAddress DS18B20Addr;// address of DS19b20 1-wire thermometer  
SensorData lastSensorData;
boolean flag_HttpSensorJob = true;// timer flag for refresh Sensor data and send; flag for use by Ticker


//DS1307 RTC1(5, 4);
//WiFiUDP ntpUDP;
//NTPClient timeClient1(ntpUDP,3*3600);
//TimeManager timeMan(RTC1, timeClient1);

PubSubClient mqtt_client(wclient, mqtt_server, mqtt_port);

void setup() {
  // put your setup code here, to run once:
  DBG_PORT.begin(115200);
  DBG_PORT.println("");
  DBG_PORT.println("Setup...");
    
  initWifi();
  initDS18B20();
  ticker.attach(60,setHttpSensorJobFlag);
}

void loop() { 
  //timeMan.loopNTP();
  if (flag_HttpSensorJob){
    flag_HttpSensorJob = false;
    Job_DHT();
    Job_DS18B20();
  }
/*  if (mqtt_client.connected()){
    mqtt_client.loop();// execute  subscribed actions
  }
*/  
  if (flagSleep){
    Serial.println("Going sleep");
    ESP.deepSleep(1*60*1000*1000,RF_DEFAULT);// 16 (D0) connect to RST
  }
}

void Job_DS18B20()
{
  DT.requestTemperatures();
  for (int i=0;i<ds18b20_count;i++){
    int c = DT.getTempC(pAddr_DS18B20[i]);
    String sTopic = sCelsiumTopic + "/DS18B20/" + getStringAddress(pAddr_DS18B20[i],8);
    DBG_PORT.println(sTopic);
    sendMQTT(sTopic,(String)lastSensorData.Celsium);
  }
}

String getStringAddress(DeviceAddress addr, int len)
{
  String tmp = "";
  for (int i = 0;i<len;i++){
    if (addr[i]<=0x0F) tmp +="0";// leading zero
    tmp += String(addr[i],HEX);
  }
  tmp.toUpperCase();
  return tmp;
}

void findAllDS18B20 (DeviceAddress* addresses, int* count)
{
   *count = 0;
   DBG_PORT.println(F("Begin search DS18B20 devices"));
   int allCount = DT.getDeviceCount();
   DBG_PORT.print(F("All device count = "));
   DBG_PORT.println(allCount);

   for (int i = 0 ; i<allCount; i++)
   {
     DeviceAddress tmpAddr;
     DT.getAddress(tmpAddr,i);
     if (DT.validFamily(tmpAddr)){
        DBG_PORT.printf("Addr%i:",i);
        printAddress(tmpAddr);
        DBG_PORT.println();
        memcpy((addresses)[i],tmpAddr,sizeof(DeviceAddress));
        (*count)++;
        if ((*count) > MAX_DS18B20_COUNT){
          DBG_PORT.println(F("Max device limit!!! Stop search")); 
          return; 
        }
     }
   }
}

void printAddress(DeviceAddress addr)
{
  for (int k=0;k<8;k++){
    DBG_PORT.printf(" %02X",addr[k] );
  }
}

void initDS18B20()
{
  // init onewire DS18b20
  //DT.setOneWire(&Wire1Port);
  DT.begin();
  //DT.getAddress(&DS18B20Addr,0);
  findAllDS18B20(pAddr_DS18B20, &ds18b20_count);
}


void initWifi()
{
  //wifi setup
  WiFiMulti.addAP("KotNet", "MyKotNet123");
  waitWiFiConnected();
  DBG_PORT.print ( "IP address: " );
  DBG_PORT.println ( WiFi.localIP() );
  WiFi.printDiag(Serial);
}

void waitWiFiConnected()
{
  int i=100;
  while((WiFiMulti.run() != WL_CONNECTED) && (i>0)){
    delay(100);
    i--;
  }
}
//
//
void setHttpSensorJobFlag()
{
  flag_HttpSensorJob = true;
  //DBG_PORT.println(">>Ticker");
}

void Job_DHT()
{
  resetSensorData();
  getSensorData_DHT();
  printSensorData_DHT();
  mqtt_sendDHTCelsium();
  mqtt_sendDHTHumidity();
}

void getSensorData_DHT()
{
  resetSensorData();
  //try DHT22
  int statusDHT = Dht.read2(PIN_DHT,&lastSensorData.Celsium,&lastSensorData.Humidity,NULL);
//  //timestamp
//  if (timeMan.flag_TimeIsOK ){
//   lastSensorData.stateTimestamp = STATE_OK;
//   lastSensorData.Timestamp = timeMan.getTimestamp();
//  }
  //Data
  if (statusDHT == SimpleDHTErrSuccess){
    lastSensorData.stateHumidity = STATE_OK;
    lastSensorData.stateCelsium = STATE_OK;
  }
}

void resetSensorData()
{
  lastSensorData.stateHumidity = STATE_ERROR;
  lastSensorData.stateCelsium = STATE_ERROR;
  lastSensorData.stateTimestamp = STATE_ERROR;
}

void mqtt_sendDHTCelsium()
{
  if ((lastSensorData.stateCelsium == STATE_OK))
  {
    sendMQTT(sCelsiumTopic + "/DHT",(String)lastSensorData.Celsium);
  }
  else
  {
    DBG_PORT.println(F("Cancel publish mqtt celsium due to errors"));
  }
}

void mqtt_sendDHTHumidity()
{
  if ((lastSensorData.stateHumidity == STATE_OK))
  {
    sendMQTT(sHumidityTopic + "/DHT",(String)lastSensorData.Humidity);
  }
  else
  {
    DBG_PORT.println(F("Cancel publish mqtt humidity due to errors"));
  }
}

void printSensorData_DHT()
{
  DBG_PORT.print(F("Time: "));
  if (lastSensorData.stateTimestamp == STATE_OK){
    DBG_PORT.print(getFormattedTime(lastSensorData.Timestamp));
  }else{
    DBG_PORT.print("!!!RTC error");
    DBG_PORT.print(getFormattedTime(lastSensorData.Timestamp));
  }
  DBG_PORT.print(" => ");
  if (lastSensorData.stateCelsium == STATE_OK){
    DBG_PORT.print(lastSensorData.Celsium);
    DBG_PORT.print("C");
  }else{
    DBG_PORT.print("!!!T_READ ERROR");
  }
  DBG_PORT.print(" | ");
  if (lastSensorData.stateHumidity == STATE_OK){
    DBG_PORT.print(lastSensorData.Humidity);
    DBG_PORT.print("%");
  }else{
    DBG_PORT.print("!!!H_READ ERROR");
  }
  DBG_PORT.println("");
}

void sendMQTT(String topicName, String value)
{
  waitWiFiConnected();
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt_client.connected()) {
      DBG_PORT.println("Connecting to MQTT server");
      if (mqtt_client.connect(MQTT::Connect("arduino_" + DeviceName).set_auth(mqtt_user, mqtt_pass))) {
        DBG_PORT.println("Connected to MQTT server");
        //mqtt_client.set_callback(callback);
        //mqtt_client.subscribe("test/led"); // подписывааемся по топик с данными для светодиода
      } else {
        DBG_PORT.println("Could not connect to MQTT server"); 
      }
    }
    if (mqtt_client.connected()){
      //mqtt_client.loop();
      mqtt_client.publish(topicName,value);
      mqtt_client.disconnect();
    }
  }
}
String getFormattedTime(unsigned long rawTime) {
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

String firstZero(int val)
{
  if (val<10){
    return "0"+(String)val;  
  }else{
    return (String)val;
  }
}

