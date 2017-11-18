#include <DS1307.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include "SensorData.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "Page_Root.h"
#include "Page_SetName.h"
#include "Page_SetDate.h"



ESP8266WebServer server ( 80 );

Ticker ticker;// task timer
DS1307 RTC1(5, 4);
OneWire Wire1Port(12);
DallasTemperature DT(&Wire1Port);
  
uint8_t DS18B20Addr;// address of DS19b20 1-wire thermometer  
SensorData lastSensorData;
boolean flag_HttpSensorJob = true;// timer flag for refresh Sensor data and send; flag for use by Ticker


String HostIP = "192.168.0.100:80";
String Url = "/TMon2/web/index.php?r=temperatures/commit";
String DeviceName = "nano";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Setup...");
  // init onewire DS18b20
  DT.setOneWire(&Wire1Port);
  DT.begin();
  DT.getAddress(&DS18B20Addr,0);
  //wifi setup
  WiFi.mode(WIFI_STA);
  WiFi.softAP("KotNet", "MyKotNet123");
  waitWiFiConnected();
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  WiFi.printDiag(Serial);
  //server setup
  serverSetup();
  //ticker
  ticker.attach(60,setHttpSensorJobFlag);
  
}

void loop() {
  if (flag_HttpSensorJob){
    flag_HttpSensorJob = false;
    HttpSensorJob();
    analogWrite(15,200);
    delay(1000);
    analogWrite(15,800);
  }
  server.handleClient();
    
  if (getNeedGoSleep()){
    ESP.deepSleep(5*60*1000*1000,RF_DEFAULT);
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

void HttpSensorJob(){
  requestTemperature();
  lastSensorData.Celsium = DT.getTempCByIndex(0); 
  if (lastSensorData.Celsium == DEVICE_DISCONNECTED_C){
    lastSensorData.stateCelsium = STATE_ERROR;
  }
  lastSensorData.Timestamp = RTC1.getTime();
  if (lastSensorData.Timestamp.mon == 0){
    lastSensorData.stateTimestamp = STATE_ERROR;
  }
  Serial.print(F("Time: "));
  if (lastSensorData.stateTimestamp == STATE_OK){
    Serial.print(getDateTimeUrl(lastSensorData.Timestamp));
  }else
  {
    Serial.print("!!!RTC error");
  }
  Serial.print(" => ");
  if (lastSensorData.stateCelsium == STATE_OK){
    Serial.print(lastSensorData.Celsium);
    Serial.println("C");
  }else{
    Serial.println("!!!T_READ ERROR");
  }
  if ((lastSensorData.stateCelsium == STATE_OK)&&(lastSensorData.stateTimestamp == STATE_OK))
  {
    sendHttpRequest();
  }else
  {
    Serial.println(F("Cancel post results to server due to errors"));
  }
}

void sendHttpRequest()
{
  waitWiFiConnected();
  
  if (WiFi.status() == WL_CONNECTED){
    String Params = "&device_name=" + DeviceName +
                  "&celsium=" + lastSensorData.Celsium +
                  "&measured_at=" + getDateTimeUrl(lastSensorData.Timestamp);
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

void waitWiFiConnected()
{
  int i=100;
  while((WiFi.status() != WL_CONNECTED) && (i>0)){
    delay(100);
    i--;
  }
}

void requestTemperature()
{
  DT.requestTemperatures();
}

String getDateTimeUrl(Time t)
{
  return  (String)t.year + 
          firstZero(t.mon) +
          firstZero(t.date) +
          firstZero(t.hour) +
          firstZero(t.min) +
          firstZero(t.sec);
}

String firstZero(int val)
{
  if (val<10){
    return "0"+(String)val;  
  }else{
    return (String)val;
  }
}
//=====================================Web Server===========================================================
void serverSetup()
{
  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
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
  data.replace("@@CURDATE@@", getDateTimeUrl(RTC1.getTime()));
  data.replace("@@LDATE@@", getDateTimeUrl(lastSensorData.Timestamp));
  data.replace("@@LCELSIUM@@", (String)lastSensorData.Celsium);
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
