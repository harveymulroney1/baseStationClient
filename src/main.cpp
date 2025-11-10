/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

#define REMOTE_IP "10.45.1.14"
IPAddress SERVER_IP (192,168,137,201);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0); 
const char* ssid = "BaseStation";
const char* password = "123123123";
WebServer server(80);
struct Sensor{
  String name;
  String ip;
  int retries = 3;
};
Sensor sensors[]={
  {"Zone 1","192.168.137.202"},
  {"Zone 2","10.45.1.15"},
  {"Zone 3","10.45.1.16"}
};
struct ZoneData{
  String temp;
  String noise;
  String light;
  String time;
  unsigned long lastUpdated;
  bool active;
};
ZoneData zoneData[3];

//Your IP address or domain name with URL path
const char* serverNameTemp = "http://"REMOTE_IP"/getTemp";
const char* serverNameHumi = "http://"REMOTE_IP"/getHumidity";
const char* serverNamePres = "http://"REMOTE_IP"/getPressure";
const char* lightsRed = "http://"REMOTE_IP"/redON";
const char* lightsRedOff = "http://"REMOTE_IP"/redOFF";
const char* lightsGreen = "http://"REMOTE_IP"/greenON";
const char* lightsGreenOff = "http://"REMOTE_IP"/greenOFF";
const char* lightsBlue = "http://"REMOTE_IP"/blueON";
const char* lightsBlueOff = "http://"REMOTE_IP"/blueOFF";
//const char* getLowPower = "http://"REMOTE_IP"/getLowPower";
//const char* lowPowerModeOn = "http://"REMOTE_IP"/lowPowerModeOn";
//const char* lowPowerModeOff = "http://"REMOTE_IP"/lowPowerModeOff";
//const char* autoLowPowerModeOn = "http://"REMOTE_IP"/autoLowPowerModeOn";
//const char* autoLowPowerModeOff = "http://"REMOTE_IP"/autoLowPowerModeOff";

String temperature;
String humidity;
String pressure;

unsigned long previousMillis = 0;
const long interval = 2000; 

//function declarations
String httpGETRequest(const char* serverName);
void connectToWifi();
String pollSensors();
void sendCORSHeaders();
void handleClimateData();
void sanityCheck();
void fetchBatteryPercentage(int device);
void getLowPower(int device);
void lowPowerModeOn(int device);
void lowPowerModeOff(int device);
void autoLowPowerModeOn(int device);
void autoLowPowerModeOff(int device);
void fetchSavedData();
void addCORS();
void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  connectToWifi();

  server.on("/getClimateData",handleClimateData);
  server.on("/getBattery/1",[=](){fetchBatteryPercentage(1);});
  server.on("/getBattery/2",[=](){fetchBatteryPercentage(2);});
  server.on("/getBattery/3",[=](){fetchBatteryPercentage(3);});
  server.on("/getLowPower/1",[=](){getLowPower(1);});
  server.on("/getLowPower/2",[=](){getLowPower(2);});
  server.on("/getLowPower/3",[=](){getLowPower(3);});
  server.on("/lowPowerModeOn/1",[=](){lowPowerModeOn(1);});
  server.on("/lowPowerModeOn/2",[=](){lowPowerModeOn(2);});
  server.on("/lowPowerModeOn/3",[=](){lowPowerModeOn(3);});
  server.on("/lowPowerModeOff/1",[=](){lowPowerModeOff(1);});
  server.on("/lowPowerModeOff/2",[=](){lowPowerModeOff(2);});
  server.on("/lowPowerModeOff/3",[=](){lowPowerModeOff(3);});
  server.on("/autoLowPowerModeOn/1",[=](){autoLowPowerModeOn(1);});
  server.on("/autoLowPowerModeOn/2",[=](){autoLowPowerModeOn(2);});
  server.on("/autoLowPowerModeOn/3",[=](){autoLowPowerModeOn(3);});
  server.on("/autoLowPowerModeOff/1",[=](){autoLowPowerModeOff(1);});
  server.on("/autoLowPowerModeOff/2",[=](){autoLowPowerModeOff(2);});
  server.on("/autoLowPowerModeOff/3",[=](){autoLowPowerModeOff(3);});
  server.on("/sanityCheck",sanityCheck);
  server.on("/fetchSavedData",fetchSavedData);
  server.begin(); // start server.
  Serial.println("HTTP Server Started");
}
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}
//conect to local access point WiFi .. provided by server
void connectToWifi() {
  WiFi.config(SERVER_IP,gateway,subnet);
  WiFi.begin(ssid, password);
  Serial.print("\nWaiting for HTTP Server ...");
  while(WiFi.status() != WL_CONNECTED) { 
    Serial.println("Client attempting to connect to http server @"+String(REMOTE_IP)+"...");
    delay(500);   //wait for 0.5 seconds
  }
  Serial.println("");
  Serial.println("HTTP server connected ..");
  Serial.print("Connected to server with IP Address: ");
  Serial.println(REMOTE_IP);
  Serial.println();

  delay(500);     //wait for 0.5 seconds
}
bool redOn = false;
bool greenOn = false;
bool blueOn = false;

void loop() {
  unsigned long currentMillis = millis();
  server.handleClient();
  if(currentMillis - previousMillis >= interval) {
     // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED ){ 
      //pollSensors();
      //httpGETRequest(lightsRed); 
      //delay(1000);   
      //httpGETRequest(lightsGreen);
      //delay(1000);
      httpGETRequest(lightsBlue);
      delay(500);
      httpGETRequest(lightsBlueOff);
      Serial.println("Lights switching on");
      //delay(1000);

      Serial.println("Fetching climate data from server...");
      
      //temperature = httpGETRequest(serverNameTemp);
      //humidity = httpGETRequest(serverNameHumi);
      //pressure = httpGETRequest(serverNamePres);
      Serial.println("Temperature: " + temperature + "'C - Humidity: " + humidity + "% - Pressure: " + pressure + " hPa");
      
      //save the last HTTP GET request
      previousMillis = currentMillis;
    }
    else {
      Serial.println("WiFi or server disconnected");
    }
  }
}
void fetchSavedData()
{
  for(int i=0;i<3;i++)
  {
    String endpoint = "http://"+String(sensors[i].ip)+"/getSavedReadings";
    String response = httpGETRequest(endpoint.c_str());
    Serial.println("Received from sensor "+sensors->name+":\n" + response);
    //STORE TO DB here ******
  }
}
void sanityCheck(){
    addCORS();
  int count = 0;
  for(int i=0;i<3;i++)
  {
    String endpoint = "http://"+String(sensors[i].ip)+"/sanityCheck";
    String response = httpGETRequest(endpoint.c_str());
    if(response = "Data OK")
    {
      count++;
    }
  }
  if(count==3)
  {
    server.send(200,"text/plain","Data OK - ALL VALID");
    
  }
  else{
    server.send(4040,"text/plain","NOT ALL VALID - Error Occured");
  }
}
void fetchBatteryPercentage(int device){
  addCORS();
  Serial.println("Getting battery");
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Getting battery from: "+host);

  String endpoint = "http://"+host+"/getBattery";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Battery:"+response);
  server.send(200,"text/plain",response);
  
}

void getLowPower(int device){
  addCORS();
  Serial.println("Getting low power");
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Getting battery from: "+host);

  String endpoint = "http://"+host+"/getLowPower";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Low power mode: "+response);
  server.send(200,"text/plain",response);
  
}


void lowPowerModeOn(int device){
  addCORS();
  Serial.println("Activating low power mode for device " + device);
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Low power mode enabling for: "+host);

  String endpoint = "http://"+host+"/lowPowerModeOn";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Low power mode on response : "+response);
  server.send(200,"text/plain",response);
  
}

void lowPowerModeOff(int device){
  addCORS();
  Serial.println("Deactivating low power mode for device " + device);
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Low power mode disabling for: "+host);

  String endpoint = "http://"+host+"/lowPowerModeOff";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Low power mode off response : "+response);
  server.send(200,"text/plain",response);
  
}

void autoLowPowerModeOn(int device){
  addCORS();
  Serial.println("Enabling automatic low power mode for device " + device);
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Enabling automatic low power mode for: "+host);

  String endpoint = "http://"+host+"/autoLowPowerModeOn";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Automatic low power mode on response : "+response);
  server.send(200,"text/plain",response);
  
}

void autoLowPowerModeOff(int device){
  addCORS();
  Serial.println("Disabling automatic low power mode for device " + device);
  String host = "";
  if(device>0 && device <=3){
    host = sensors[device-1].ip;
    // boundary
  }
  else{
    return;
  }
    Serial.println("Disabling automatic low power mode for: "+host);

  String endpoint = "http://"+host+"/autoLowPowerModeOff";
  String response = httpGETRequest(endpoint.c_str());
  Serial.println("Automatic low power mode off response : "+response);
  server.send(200,"text/plain",response);
}


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  //your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  //send HTTP POST request
  int httpResponseCode = http.GET();
  String payload = "--"; 

  if (httpResponseCode>0) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  //free resources
  http.end();

  return payload;
}
std::vector<String> split(String s, char delimiter) {
  std::vector<String> tokens;
  int start = 0;
  int end = s.indexOf(delimiter);
  while (end != -1) {
    tokens.push_back(s.substring(start, end));
    start = end + 1;
    end = s.indexOf(delimiter, start);
  }
  tokens.push_back(s.substring(start));
  return tokens;
}
void handleClimateData()
{
  addCORS();
  String response = pollSensors();
  server.send(200,"text/plain",response);
  return;
  
}
String pollSensors(){
  WiFiClient client;
  HTTPClient http;
  String payload = ""; 
  //your Domain name with URL path or IP address with path
  for(int i=0;i<3;i++)
  {
    const String sensorClimate = "http://"+String(sensors[i].ip)+"/getClimateData";
    Serial.println("Requesting from: "+sensors[i].ip);
    http.begin(client,sensorClimate);
    //send HTTP POST request
    int httpResponseCode = http.GET();
    

    if(httpResponseCode==200){
      String response = http.getString();
      auto parts = split(response,',');
      if (parts.size() >= 5) 
      {
        zoneData[i].temp = parts[1];
        zoneData[i].noise = parts[2];
        zoneData[i].light = parts[3];
        zoneData[i].time = parts[4];
        zoneData[i].lastUpdated = millis();
        zoneData[i].active = true;
        Serial.println("Response:"+response);
        payload+= response + "\n";
      }
    else if(sensors[i].retries==0){
      // retry mechanism
      zoneData[i].active=false;
    }
    else {
      Serial.print("Error code: ");
      sensors[i].retries-=1; // reduce by 1
      Serial.println(httpResponseCode);
    }
    //free resources
    http.end();
    
  }
}
  return payload;
}
