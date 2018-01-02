#include <NTPClient.h>

#include <DS1307.h>
#include <DallasTemperature.h>
#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include "SensorData.h"
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>

#include <WiFiUdp.h>

SimpleDHT22 Dht;
ESP8266WiFiMulti WiFiMulti;

#include "Page_Root.h"
#include "Page_SetName.h"
#include "Page_SetDate.h"

/*
4 (D2)  - RTC
5 (D1)  - RTC 
12(D6) - 1-Wire
*/
#define PIN_1WIRE 12

String HostIP = "192.168.0.100:80";
String Url = "/TMon2/web/index.php?r=temperatures/commit";
String DeviceName = "nano";

ESP8266WebServer server ( 80 );

Ticker ticker;// task timer
DS1307 RTC1(5, 4);
OneWire Wire1Port(PIN_1WIRE);
DallasTemperature DT(&Wire1Port);
  
uint8_t DS18B20Addr;// address of DS19b20 1-wire thermometer  
SensorData lastSensorData;
boolean flag_HttpSensorJob = true;// timer flag for refresh Sensor data and send; flag for use by Ticker

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,3*3600);
bool flag_TimeIsOK = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Setup...");
  initDS18B20();
  initWifi();
  initWebServer();
  //ticker
  ticker.attach(60,setHttpSensorJobFlag);
  timeClient.setUpdateInterval(60000*60);
  flag_TimeIsOK = timeClient.forceUpdate();
}

void loop() {
  flag_TimeIsOK = timeClient.update();
  server.handleClient();
  if (flag_HttpSensorJob){
    flag_HttpSensorJob = false;
    HttpSensorJob();
  }
  if (getNeedGoSleep()){
    Serial.println("Going sleep");
    ESP.deepSleep(5*60*1000*1000,RF_DEFAULT);// 16 (D0) connect to RST
  }
}

void initDS18B20()
{
  // init onewire DS18b20
  DT.setOneWire(&Wire1Port);
  DT.begin();
  DT.getAddress(&DS18B20Addr,0);
}
void initWifi()
{
  //wifi setup
  WiFiMulti.addAP("KotNet", "MyKotNet123");
  waitWiFiConnected();
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
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
boolean getNeedGoSleep()
{
  return false;
}

void setHttpSensorJobFlag()
{
  flag_HttpSensorJob = true;
}

void getSensorData()
{
  lastSensorData.stateHumidity = STATE_ERROR;
  lastSensorData.stateCelsium = STATE_ERROR;
  //try DHT22
  byte t;
  byte h;
  int statusDHT = Dht.read(PIN_1WIRE,&t,&h,NULL);
  //timestamp
  if ( !flag_TimeIsOK ){
    lastSensorData.stateTimestamp = STATE_ERROR;
  }else{
    lastSensorData.stateTimestamp = STATE_OK;
    lastSensorData.Timestamp = timeClient.getEpochTime();
  }
  //Data
  if (statusDHT == SimpleDHTErrSuccess){
    lastSensorData.stateHumidity = STATE_OK;
    lastSensorData.stateCelsium = STATE_OK;
    lastSensorData.Celsium = t;
    lastSensorData.Humidity = h;
  }else{
    // try DS18b20
    requestTemperature();
    lastSensorData.Celsium = DT.getTempCByIndex(0); 
    if (lastSensorData.Celsium != DEVICE_DISCONNECTED_C){
      lastSensorData.stateCelsium = STATE_OK;
    }
  }
}

void HttpSensorJob()
{
  getSensorData();
  Serial.print(F("Time: "));
  if (lastSensorData.stateTimestamp == STATE_OK){
    Serial.print(getFormattedTime(lastSensorData.Timestamp));
  }else{
    Serial.print("!!!RTC error");
  }
  Serial.print(" => ");
  if (lastSensorData.stateCelsium == STATE_OK){
    Serial.print(lastSensorData.Celsium);
    Serial.print("C");
  }else{
    Serial.print("!!!T_READ ERROR");
  }
  Serial.print(" | ");
  if (lastSensorData.stateHumidity == STATE_OK){
    Serial.print(lastSensorData.Humidity);
    Serial.print("%");
  }else{
    Serial.print("!!!H_READ ERROR");
  }
  Serial.println("");
  if ((lastSensorData.stateCelsium == STATE_OK)&&(lastSensorData.stateTimestamp == STATE_OK))
  {
    sendHttpRequest();
  }else{
    Serial.println(F("Cancel post results to server due to errors"));
  }
}

void sendHttpRequest()
{
  waitWiFiConnected();
  
  if (WiFi.status() == WL_CONNECTED){
    String Params = "&device_name=" + DeviceName +
                  "&celsium=" + lastSensorData.Celsium +
                  "&measured_at=" + lastSensorData.Timestamp;
    String SendUrl = "http://" + HostIP + Url + Params; 
    Serial.println("Try send data");
       
    Serial.println(SendUrl);
    
    HTTPClient http;
    http.begin(SendUrl);
    int httpCode = http.GET();
    if (httpCode >0){
      Serial.println("DataSend result: " + (String)httpCode);
    }else{
      Serial.println("DataSend result error: " + (String)http.errorToString(httpCode));
    }
    http.end();
  }else{
    Serial.println(F("Error: wifi not connected."));
  }
}

void requestTemperature()
{
  DT.requestTemperatures();
}

String getNowTimeStr()
{
  if ( flag_TimeIsOK )
  {
    return "<time error>";
  }
  else
  {
    return timeClient.getFormattedTime();
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
//String convertTimeToUrlStr(Time t)
//{
//  return  (String)t.year + 
//          firstZero(t.mon) +
//          firstZero(t.date) +
//          firstZero(t.hour) +
//          firstZero(t.min) +
//          firstZero(t.sec);
//}

String firstZero(int val)
{
  if (val<10){
    return "0"+(String)val;  
  }else{
    return (String)val;
  }
}
//=====================================Web Server===========================================================
void initWebServer()
{
  //if ( MDNS.begin ( "esp8266" ) ) {
  //  Serial.println ( "MDNS responder started" );
  //}
  server.on ( "/", handleRoot );
  server.on ( "/setname", handleSetName );
  server.on ( "/setdate", handleSetDate );
  server.onNotFound ( handleNotFound );
  server.begin();  
}

void handleRoot() 
{
  String data = ROOT_page;
  data.replace("@@DEVNAME@@", DeviceName);
  data.replace("@@CURDATE@@", getNowTimeStr());
  data.replace("@@LDATE@@", getFormattedTime(lastSensorData.Timestamp));
  data.replace("@@LCELSIUM@@", (String)lastSensorData.Celsium);
  data.replace("@@LHUMIDITY@@", (String)lastSensorData.Humidity);
  server.send ( 200, "text/html; charset=utf-8", data );
}

void handleSetName()
{
  String data = SETNAME_page;
  if (server.method() == HTTP_POST){
    // form submit
    String newName = server.arg("devname");
    if (newName != ""){
      DeviceName = newName;
    }
    data.replace("@@RESULT@@", "Data were saved.");
  }
  data.replace("@@RESULT@@", "");
  data.replace("@@DEVNAME@@", DeviceName);
  server.send ( 200, "text/html; charset=utf-8", data );
}

void handleSetDate()
{
  String data = SETDATE_page;
  if (server.method() == HTTP_POST){
    RTC1.setDate(server.arg("day").toInt(), server.arg("month").toInt(), server.arg("year").toInt());
    RTC1.setTime(server.arg("hour").toInt(), server.arg("minute").toInt(),0);
    data.replace("@@RESULT@@", "Date was set.");
  }
  Time t = RTC1.getTime();
  data.replace("@@RESULT@@", "");
  data.replace("@@YEAR@@", (String)t.year);
  data.replace("@@MONTH@@", (String)t.mon);
  data.replace("@@DAY@@", (String)t.date);
  data.replace("@@HOUR@@", (String)t.hour);
  data.replace("@@MINUTE@@", (String)t.min);
  
  server.send ( 200, "text/html; charset=utf-8", data );
}

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}
