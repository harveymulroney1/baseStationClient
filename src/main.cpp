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
IPAddress SERVER_IP (10,45,1,13);
IPAddress gateway(10, 45, 1, 1);
IPAddress subnet(255, 255, 255, 0); 
const char* ssid = "GowersSmall";
const char* password = "mattyisalegend";
WebServer server(80);
struct Sensor{
  String name;
  String ip;
  int retries = 3;
};
Sensor sensors[]={
  {"Zone 1","10.45.1.14"},
  {"Zone 2","10.45.1.15"},
  {"Zone 3","10.45.1.16"}
};
struct ZoneData{
  String temp;
  String noise;
  String light;
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
      pollSensors();
      httpGETRequest(lightsRed); 
      delay(1000);   
      httpGETRequest(lightsGreen);
      delay(1000);
      httpGETRequest(lightsBlue);
      delay(500);
      httpGETRequest(lightsBlueOff);
      Serial.println("Lights switching on");
      delay(1000);

      Serial.println("Fetching climate data from server...");
      
      temperature = httpGETRequest(serverNameTemp);
      humidity = httpGETRequest(serverNameHumi);
      pressure = httpGETRequest(serverNamePres);
      Serial.println("Temperature: " + temperature + "'C - Humidity: " + humidity + "% - Pressure: " + pressure + " hPa");
      
      //save the last HTTP GET request
      previousMillis = currentMillis;
    }
    else {
      Serial.println("WiFi or server disconnected");
    }
  }
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
    
  //your Domain name with URL path or IP address with path
  for(int i=0;i<3;i++)
  {
    const String sensorClimate = "http://"+String(sensors[i].ip)+"/getClimateData";
    Serial.println("Requesting from: "+sensors[i].ip);
    http.begin(client,sensorClimate);
    //send HTTP POST request
    int httpResponseCode = http.GET();
    String payload = "--"; 

    if(httpResponseCode==200){
      payload = http.getString();
      zoneData[i].lastUpdated=millis();
      zoneData[i].active=true;
      Serial.println("Response:"+payload);
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
    return payload;
  }
}