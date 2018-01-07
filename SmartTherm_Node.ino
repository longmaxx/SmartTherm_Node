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

#include <FS.h>
File fsUploadFile;

SimpleDHT22 Dht;
ESP8266WiFiMulti WiFiMulti;

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

#define DBG_OUTPUT_PORT Serial

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
} 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Setup...");
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  
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
    return "=time error=";
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

//===================================== SPIFFS==============================================================
String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

//=====================================Web Server===========================================================
void initWebServer()
{
  //if ( MDNS.begin ( "esp8266" ) ) {
  //  Serial.println ( "MDNS responder started" );
  //}
  //server.on ( "/", handleRoot );
  //server.on ( "/setname", handleSetName );
  //server.on ( "/setdate", handleSetDate );
  server.on("/last",HTTP_GET, handleAjaxDataGet);
    //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound ( handleNotFound );
  server.begin();  
}

void handleAjaxDataGet()
{
  String data = "{";

   data +="\"devname\":\"" + DeviceName + "\"" + ",";
   data +="\"celsium\":\"" + (String)lastSensorData.Celsium + "\",";
   data +="\"humidity\":\"" + (String)lastSensorData.Humidity + "\",";
   data +="\"date_now\":\"" + getNowTimeStr() + "\",";
   data +="\"date_last\":\"" + getFormattedTime(lastSensorData.Timestamp) + "\"";

   data +="}";
    DBG_OUTPUT_PORT.println(data);
   server.send(200,"application/json",data);
}

void handleNotFound() 
{
  if (!handleFileRead(server.uri())){
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
}

//============================================web spiffs===================================================
void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}
