#include <ArduinoJson.h>

#include <DallasTemperature.h>
#include <SimpleDHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "SensorData.h"
//#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "Options.h"
#include <FS.h>

bool flagSleep = false;
Ticker ticker;// task timer

String HostIP = "192.168.1.110:80";
String Url = "/web/index.php?r=temperatures/commit";
String DeviceName = "nano";


//String wifiName = "KotNet";
//String wifiPwd  = "MyKotNet123";


char wifiAPName[] = "AP_nano";//+DeviceName;
char wifiAPPwd[] = "12345678";
bool flag_EnableAP = false;

String sCelsiumTopic = DeviceName + "/Celsium";
String sHumidityTopic = DeviceName + "/Humidity";

SimpleDHT22 Dht;
//ESP8266WiFiMulti WiFiMulti;
WiFiClient wclient;

ESP8266WebServer server(80);
File fsUploadFile;

/*
12(D6) - 1-Wire ( DHT22)
14() - 1-wire DS18b20 
16(D0) connect to RST for SLEEP WAKE UP work
13 - Btn Config. Pull-Up to enable web interface
*/
#define PIN_DHT (12)
#define PIN_1WIRE (14)
#define PIN_BTN_CONFIG (13)
#define DBG_PORT Serial

#define PIN_BTN_STATE_ENABLED (1)

OneWire Wire1Port(PIN_1WIRE);
DallasTemperature DT(&Wire1Port);
char oneWire_Phaze = 0;
#define ONEWIRE_PHAZE_IDLE (0)
#define ONEWIRE_PHAZE_MEASURING (1)
#define ONEWIRE_PHAZE_READY (2)
DHTSensorData lastSensorData;
  
DeviceAddress DS18B20Addr;// address of DS19b20 1-wire thermometer  
boolean flag_HttpSensorJob = true;// timer flag for refresh Sensor data and send; flag for use by Ticker


//DS1307 RTC1(5, 4);
//PubSubClient mqtt_client(wclient, mqtt_server, mqtt_port);
//PubSubClient mqtt_client((uint8_t*)mqtt_server,(uint16_t) mqtt_port, (Client)wclient);
PubSubClient mqtt_client(wclient);
boolean mqtt_enabled = false;

void setup() {
  pinMode(PIN_BTN_CONFIG,INPUT);
  // put your setup code here, to run once:
  DBG_PORT.begin(115200);
  DBG_PORT.println("");
  DBG_PORT.println("Setup...");
  SPIFFS.begin();
  checkBtnConfigState(); 
  initMQTT();
  initWifi();
  initDS18B20();
  ticker.attach(30,setHttpSensorJobFlag);
}

void loop() { 
  server.handleClient();
  if (flag_HttpSensorJob){
    flag_HttpSensorJob = false;
    if(mqtt_enabled)
      mqttConnect();
    Job_DHT();
    Job_DS18B20();
  }
  if (oneWire_Phaze != ONEWIRE_PHAZE_IDLE)
    Job_DS18B20();
  if (mqtt_client.connected()){
    mqtt_client.loop();// execute  subscribed actions
  }
  if (flagSleep){
    Serial.println("Going sleep");
    ESP.deepSleep(1*60*1000*1000,RF_DEFAULT);// 16 (D0) connect to RST
  }
}

void checkBtnConfigState()
{
  if (digitalRead(PIN_BTN_CONFIG) == PIN_BTN_STATE_ENABLED){
    delay(3000);
    if (digitalRead(PIN_BTN_CONFIG) == PIN_BTN_STATE_ENABLED){
      flag_EnableAP = true;
    }
  }
}

void readWifiSettingsFromFlash()
{
  readFileLine(opt_wifi_file, opt_wifi_ssid_i, ssidWiFi, WIFI_SSID_LEN_MAX);
  readFileLine(opt_wifi_file, opt_wifi_password_i, passwordWiFi, WIFI_PASSWORD_LEN_MAX);
}
void readMqttSettingsFromFlash()
{
  readFileLine(opt_mqtt_file, opt_mqtt_server_i, mqtt_server, MQTT_SERVER_LEN_MAX);
  readFileLine(opt_mqtt_file, opt_mqtt_port_i, mqtt_port, WIFI_SSID_LEN_MAX);
  readFileLine(opt_mqtt_file, opt_mqtt_user_i, mqtt_user, MQTT_USER_LEN_MAX);
  readFileLine(opt_mqtt_file, opt_mqtt_password_i, mqtt_password, MQTT_PASSWORD_LEN_MAX);
}

void Job_DS18B20()
{
  switch (oneWire_Phaze)
  {
    case ONEWIRE_PHAZE_IDLE :
        oneWire_Phaze = ONEWIRE_PHAZE_MEASURING;
        DT.requestTemperatures();
        break;
    case ONEWIRE_PHAZE_MEASURING : 
        if (DT.isConversionComplete())
           oneWire_Phaze = ONEWIRE_PHAZE_READY;  
        break;
    case ONEWIRE_PHAZE_READY :    
        for (int i=0;i<DS18B20_count;i++)
        {
          int c = DT.getTempC(DS18B20_Values[i].address);
          DS18B20_Values[i].celsium = c;
          String sTopic = sCelsiumTopic + "/DS18B20/" + getStringAddress(DS18B20_Values[i].address,8);
          DBG_PORT.println(sTopic + " : " + c);
          sendMQTT(sTopic,(String)c);
        }
        oneWire_Phaze = ONEWIRE_PHAZE_IDLE;
        break;
    default: 
        DBG_PORT.println("ERROR! Unknown OneWire phaze");
        oneWire_Phaze = ONEWIRE_PHAZE_IDLE;    
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

void findAllDS18B20 ()
{
   DS18B20_count = 0;
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
        DBG_PORT.print(getStringAddress(tmpAddr,8));
        DBG_PORT.println();
        memcpy(DS18B20_Values[i].address,tmpAddr,sizeof(DeviceAddress));
        (DS18B20_count)++;
        if (DS18B20_count > MAX_DS18B20_COUNT){
          DBG_PORT.println(F("Max device limit!!! Stop search")); 
          return; 
        }
     }
   }
}

void initDS18B20()
{
  // init onewire DS18b20
  //DT.setOneWire(&Wire1Port);
  oneWire_Phaze = ONEWIRE_PHAZE_IDLE;
  DT.begin();
  DT.setWaitForConversion(false);
  //DT.getAddress(&DS18B20Addr,0);
  findAllDS18B20();
}

void initMQTT()
{
  if (mqtt_enabled)
  {
    readMqttSettingsFromFlash();
    String srv(mqtt_server);
    mqtt_client.set_server(srv,String(mqtt_port).toInt());
  }
}

void initWifi()
{
    readWifiSettingsFromFlash();
  //wifi setup
  //if (flag_EnableAP){
    flag_EnableAP = false;
    //WiFi.softAP(wifiAPName,wifiAPPwd);
    initWebServer();
  //}else{
    //WiFi.persistent(false);
    WiFi.begin(ssidWiFi, passwordWiFi);
    waitWiFiConnected();
    DBG_PORT.print ( "IP address: " );
    DBG_PORT.println ( WiFi.localIP() );
    WiFi.printDiag(Serial);
  //}
}

void waitWiFiConnected()
{
  int i=100;
  while((WiFi.status() != WL_CONNECTED) && (i>0)){
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
  int statusDHT = Dht.read2(PIN_DHT,&lastSensorData.DHTCelsium,&lastSensorData.DHTHumidity,NULL);
  //Data
  if (statusDHT == SimpleDHTErrSuccess){
    lastSensorData.stateDHT = STATE_OK;
  }
}

void resetSensorData()
{
  lastSensorData.stateDHT = STATE_ERROR;
  lastSensorData.stateTimestamp = STATE_ERROR;
}

void mqtt_sendDHTCelsium()
{
  if ((lastSensorData.stateDHT == STATE_OK))
  {
    sendMQTT(sCelsiumTopic + "/DHT",(String)lastSensorData.DHTCelsium);
  }
  else
  {
    DBG_PORT.println(F("Cancel publish mqtt celsium due to errors"));
  }
}

void mqtt_sendDHTHumidity()
{
  if ((lastSensorData.stateDHT == STATE_OK))
  {
    sendMQTT(sHumidityTopic + "/DHT",(String)lastSensorData.DHTHumidity);
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
  if (lastSensorData.stateDHT == STATE_OK){
    DBG_PORT.print(lastSensorData.DHTCelsium);
    DBG_PORT.print("C");
  }else{
    DBG_PORT.print("!!!T_READ ERROR");
  }
  DBG_PORT.print(" | ");
  if (lastSensorData.stateDHT == STATE_OK){
    DBG_PORT.print(lastSensorData.DHTHumidity);
    DBG_PORT.print("%");
  }else{
    DBG_PORT.print("!!!H_READ ERROR");
  }
  DBG_PORT.println("");
}

void mqttConnect()
{
  if (!mqtt_client.connected()) {
      DBG_PORT.println("Connecting to MQTT server");
      if (mqtt_client.connect(MQTT::Connect("arduino_" + DeviceName).set_auth(mqtt_user, mqtt_password))) {
        DBG_PORT.println("Connected to MQTT server");
        //mqtt_client.set_callback(callback);
        //mqtt_client.subscribe("test/led"); // подписывааемся по топик с данными для светодиода
      } else {
        DBG_PORT.println("Could not connect to MQTT server"); 
      }
    }
}

void sendMQTT(String topicName, String value)
{
  if (mqtt_enabled)
  {
    waitWiFiConnected();
    if (WiFi.status() == WL_CONNECTED) {
      if (mqtt_client.connected()){
        //mqtt_client.loop();
        mqtt_client.publish(topicName,value);
        mqtt_client.disconnect();
      }
    }
    DBG_PORT.println("MQTT is disabled");
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
//====================== SPIFFS ========================================
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}
String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
bool handleFileRead(String path){
  DBG_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
//================options to flash======================================
void saveOptionToFlash(String fileName, String textOpt)
{
  SPIFFS.remove(fileName);
  File file = SPIFFS.open(fileName, "w+");
  file.println(textOpt);
  file.close();
}

int readFileLine(String fileName, int iLine, char* buf, int len)
{
  File file = SPIFFS.open(fileName, "r");
  bool hasLine = fileGotoLine(file, iLine);
  int lenRead = 0;
  if (hasLine)
    lenRead = file.readBytesUntil('\r', buf, len);
  else {
    buf[0] = '\0';
    //DBG_PORT.print("no line");
  }
  file.close();
  DBG_PORT.write("Read ");
  DBG_PORT.print(fileName);
  DBG_PORT.write(" line: ");
  String s = String(*buf);
  DBG_PORT.write((uint8_t*)buf,lenRead);
  DBG_PORT.write("\r\n");
  return lenRead;
}

bool fileGotoLine(File file, int iLine)
{
  int len = 50;
  char buf[50];
  file.seek(0,SeekSet);
  if (iLine == 0) return true;
  for (int i = 0; i < iLine; i++)
  {
    //DBG_PORT.write("GOTOLine=");
    //DBG_PORT.print(i, DEC);
    int l = file.readBytesUntil('\n',buf,len);
    //DBG_PORT.write((uint8_t*)buf,l);
    //DBG_PORT.println("");
    if (file.position() == file.size()) {
      //DBG_PORT.println("Fail. End of file!");
      return false;
    }
  }
  return true;
}
//=======================WebServer======================================
void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_PORT.print("handleFileUpload Name: "); DBG_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_PORT.print("handleFileUpload Data: "); DBG_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_PORT.print("handleFileUpload Size: "); DBG_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

void handleGetWiFi()
{
  DynamicJsonDocument  docJSON(300);
  JsonObject root = docJSON.to<JsonObject>();
  JsonArray arrNetworks =root.createNestedArray("networks"); 
  int count = WiFi.scanNetworks();
  DBG_PORT.println("Networks count:"+String(count));
  for (int i=0;i<count;i++)
  {
    DBG_PORT.println(WiFi.SSID(i));
    arrNetworks.add(WiFi.SSID(i));
  }
  root["connected"] = wclient.connected();
  root["ID"] = String(ssidWiFi);
  String result;
  serializeJsonPretty(root,result);
  server.send(200,"text/json",result) ;
}
void handleSetWiFi()
{
  DBG_PORT.println("handleSetWiFi");
//  DBG_PORT.println(server.arg("name"));
  if (server.hasArg("ssid") && (server.hasArg("password"))){
    if ((server.arg("ssid").length() >=20)||(server.arg("password").length() >=30)){
      server.send(500,"text/json","{'success': false,'error':'Too long credentials'}");
      return;
    }
    //server.arg("ssid").toCharArray(ssidWiFi,20);
    //server.arg("password").toCharArray(passwordWiFi,30);
    saveOptionToFlash(opt_wifi_file,server.arg("ssid") + "\r\n" + server.arg("password"));
    initWifi();
    handleGetWiFi();
  }else if (server.args() >0){
    server.send(400,"text/json","{'success': 'false','error':'Bad request. Need query args: name, pwd.'}");
  }
}

void handleGetMQTT()
{
  String url(mqtt_server);
  String port(mqtt_port);
  String login(mqtt_user);
  String pwd(mqtt_password);
  //DBG_PORT.println("name: "+ssid);
  //DBG_PORT.println("pwd: "+pwd);
  server.send(200,"text/json","{'server':'" + url + "','port':'" + port + "','login':'" + login +"','password':'" + pwd +"'}") ;  
}

void handleSetMQTT()
{
  server.send(501,"text/json","Not Implemented") ;  
}

void handleGetWebRoot()
{
  handleFileRead("/index.html");
}

void handleGetData()
{
  DynamicJsonDocument  docJSON(500);
  JsonObject root = docJSON.to<JsonObject>();
  root["name"] = DeviceName;
  JsonObject dht = root.createNestedObject("DHT");
  if (lastSensorData.stateDHT == STATE_OK){
    dht["Celsium"] = lastSensorData.DHTCelsium;
    dht["Humidity"] = lastSensorData.DHTHumidity;
  }
  JsonObject ds = root.createNestedObject("DS18B20");
  for (int i=0;i<DS18B20_count;i++)
  {
    String dsId = getStringAddress(DS18B20_Values[i].address,8);
    JsonObject dsObj = ds.createNestedObject(dsId);
    dsObj["celsium"] = DS18B20_Values[i].celsium;
    dsObj["name"] = getDs18b20Alias(dsId);
  }
  String result;
  serializeJsonPretty(root,result);
  DBG_PORT.println(result);
  server.send(200,"text/json",result);
}

void handleGetDS18B20Alias()
{
  if (  (!server.hasArg(arg_ds18b20_id)) || 
        ((!server.hasArg(arg_ds18b20_id)) && (!server.hasArg(arg_ds18b20_name)) ) 
  ){
    server.send(400,"text/json","{'success': 'false','error':'Bad request. Need query args: " + String(arg_ds18b20_id) + ", "+String(arg_ds18b20_name)+"'}");
  }

  if (server.hasArg(arg_ds18b20_name)){
    // we save new name
    DBG_PORT.println("Saving DS18b20 alias: "+server.arg(arg_ds18b20_id) + " = " +server.arg(arg_ds18b20_name) );
    SPIFFS.remove(opt_ds18b20_alias_files + server.arg(arg_ds18b20_id));
    if (server.arg(arg_ds18b20_name) != "")// don't create file fr empty name
    {
      File f = SPIFFS.open(opt_ds18b20_alias_files + server.arg(arg_ds18b20_id),"w+");
      f.print(server.arg(arg_ds18b20_name));
      f.close();
    }
    server.send(200,"text/json","{'success': true}");
  }else{
    String alias = getDs18b20Alias(server.arg(arg_ds18b20_id));
    DBG_PORT.println("Just getting an ID for DS18b20...");
    server.send(200,"text/json","{'name':'"+alias+"'}");
  }
}

String getDs18b20Alias(String id)
{
    char buf[20];
    buf[0] = '\0';
    String path = opt_ds18b20_alias_files + id;
    if (SPIFFS.exists(path))
    {
      File f = SPIFFS.open(path, "r");
      if (f)
      {
        int len = f.read((uint8_t*)buf,20);
        buf[len] = '\0';
        DBG_PORT.print("Read alias (" + id + ") :");      
        DBG_PORT.write((uint8_t*)buf,len);
        DBG_PORT.println("");
        f.close();
      }
      else
      {
        DBG_PORT.println("Failed to  open alias file: " + path);    
      }
    }
    return String(buf);
}
void initWebServer()
{
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_PORT.printf("\n");
  }
    //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
          if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
          });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
  server.on("/",HTTP_GET,handleGetWebRoot);
    //server.on("/list", HTTP_GET, handleFileList);
  server.on ("/wifi",HTTP_POST,handleSetWiFi);
  server.on ("/wifi",HTTP_GET, handleGetWiFi);
  server.on ("/mqtt",HTTP_POST,handleSetMQTT);
  server.on ("/mqtt",HTTP_GET, handleGetMQTT);
  server.on ("/data",HTTP_GET, handleGetData);
  server.on("/ds18b20/alias",HTTP_GET,handleGetDS18B20Alias);
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  server.begin();
}
