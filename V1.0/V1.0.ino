#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>

//sfhtreg 74hc595
#define sftData 12     //Ds PIN
#define sftClock 11   //Sh_CP PIN
#define sftLatch 10  //ST_cp conectar cap101(100pf) a GND

#define speakerPin 8

SoftwareSerial WIFI(3, 2); // RX | TX

//Sensor press temperatura
Adafruit_BMP085 BMP;

//Sensor humedad - Temperatura
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Rain Sensor
#define rainPin 1
int rainVal=0;

char linkId;
char serialData[256]; //entrada - salida puerto serie
int lngData;
char jsonData[150];
char wifiCmd [20];
String cadena;

byte shftVal = 0;

//Tiempo entre lecturas default
#define lapseReadings 900000 //15Minutos
//#define lapseReadings 600000 //10Minutos
//#define lapseReadings 300000 //5Minutos
//#define lapseReadings 100000
unsigned long millisLoop;
unsigned long startMillis;


void setup()
{
  pinMode(sftLatch, OUTPUT);   // make the data pin an output
  digitalWrite(sftLatch, LOW); // apago shift
  pinMode(sftClock, OUTPUT);    // make the clock pin an output
  pinMode(sftData, OUTPUT);     // make the data pin an output

  Serial.begin(9600);
  
  pinMode(speakerPin, OUTPUT);
  pinMode(rainPin, INPUT);


  WIFI.begin(14400);
  Serial.println(F("REESTARTING WIFI..."));
  WIFI.println(F("AT+RST")); //ready
                             //WIFI CONNECTED
                             //WIFI GOT IP
  waitESPGotIp();
  WIFI.println(F("ATE0"));
  Serial.println(F("STARTING WIFI SERVER..."));
  startWifiServer();

  Serial.println(F("STARTING TEMP HUM SENSOR..."));
  dht.begin();

  Serial.println(F("STARTING TEMP PRESS SENSOR..."));
  //inicio BMP180 presion - temperatura
  if (!BMP.begin())
    Serial.println(F("Error inicializando BMP180, check wiring!"));
  delay(200);

  Serial.println(F("Loading Settings..."));
  shftVal=EEPROM.read(0);

  sendToShift();
  Serial.println(F("..NOW IAM BREATHING.."));

  blip();
  
  millisLoop=millis();
}
void (*resetFunc)(void) = 0; //reset function @ address 0
void loop()
{
  /*
    if (WIFI.available())                // Lo que entra por WIFI à Serial
         { w = WIFI.read() ;
           Serial.print(w);

         }
      if (Serial.available())             // Lo que entra por Serial à WIFI
         {  char s = Serial.read();
            WIFI.print(s);
            
         }*/

  if (WIFI.available())
  {
    memset(serialData, 0, sizeof serialData);
    WIFI.readBytesUntil('\n', serialData, 100);
    if (strstr(serialData, "+IPD"))
      recibeWifiData();
    else if (strstr(serialData, "ready"))
      resetFunc();
  }
  rainVal = analogRead(rainPin);

  if((millis()-millisLoop)>lapseReadings){
      wifiPostDataA();
      millisLoop=millis();
  }
}

void recibeWifiData()
{
  linkId = serialData[5];

  if (strstr(serialData, "porton"))
    if (strstr(serialData, "action=1"))
      openGate();
    else
      closeGate();
  else if (strstr(serialData, "salida"))
    togleSalidas();
  else if (strstr(serialData, "sensors"))
  {
    readSensors();
    responseHTTPGet();
  }
}

void wifiPostDataA(){
    readSensors();

    WIFI.flush();
    WIFI.println(F("AT+CIPSTART=0,\"TCP\",\"monitorv1.herokuapp.com\",80"));//start a TCP connection. 
    //waitStringWifi("OK",false);
    waitESPCmdEnd();

    int lngRequest=sprintf_P(serialData,PSTR("POST /api/stationReadings HTTP/1.1\r\n"
                                    //"Host: 192.168.1.44:3000\r\n"
                                    "Host: monitorv1.herokuapp.com\r\n"
                                    "Connection: keep-alive\r\n"
                                    "Content-Length: %d\r\n"
                                    "Content-Type: application/json\r\n"
                                    "Accept: */*\r\n"
                                    "Accept-Encoding: gzip, deflate, br\r\n"
                                    "Accept-Language: es-ES,es;q=0.8\r\n"
                                    "\r\n%s"),strlen(jsonData),jsonData);
    //Serial.println(serialData);

    sprintf_P(wifiCmd,PSTR("AT+CIPSEND=0,%d"),lngRequest);
    WIFI.flush();
    WIFI.println(wifiCmd);
    //    waitStringWifi(">",false);
    waitESPCmdEnd();
    
    WIFI.flush();
    WIFI.println(serialData);
    //waitStringWifi("SEND OK",false);
    waitESPCmdEnd();
    
    WIFI.flush();
    WIFI.println(F("AT+CIPCLOSE=0")); 
    //waitESPCmdEnd();
 }

void togleSalidas()
{
  char *pch;
  pch = strstr(serialData, "?");

  if (strstr(pch, "s1=1"))
    bitSet(shftVal, 0);
  if (strstr(pch, "s1=0"))
    bitClear(shftVal, 0);
  if (strstr(pch, "s2=1"))
    bitSet(shftVal, 1);
  if (strstr(pch, "s2=0"))
    bitClear(shftVal, 1);
  if (strstr(pch, "s3=1"))
    bitSet(shftVal, 2);
  if (strstr(pch, "s3=0"))
    bitClear(shftVal, 2);
  if (strstr(pch, "s4=1"))
    bitSet(shftVal, 3);
  if (strstr(pch, "s4=0"))
    bitClear(shftVal, 3);
  if (strstr(pch, "s5=1"))
    bitSet(shftVal, 4);
  if (strstr(pch, "s5=0"))
    bitClear(shftVal, 4);
  if (strstr(pch, "s6=1"))
    bitSet(shftVal, 5);
  if (strstr(pch, "s6=0"))
    bitClear(shftVal, 5);
  if (strstr(pch, "s7=1"))
    bitSet(shftVal, 6);
  if (strstr(pch, "s7=0"))
    bitClear(shftVal, 6);
  if (strstr(pch, "s8=1"))
    bitSet(shftVal, 7);
  if (strstr(pch, "s8=0"))
    bitClear(shftVal, 7);

  sendToShift();
  EEPROM.update(0,shftVal);
  memset(jsonData, 0, sizeof jsonData);
  lngData = sprintf_P(jsonData, PSTR("{\"salida\":%d,\"action\":1,\"status\":\"OK\"}"), shftVal);
  responseHTTPGet();
}

void openGate()
{

  memset(jsonData, 0, sizeof jsonData);
  lngData = sprintf_P(jsonData, PSTR("{\"command\":\"porton\",\"action\":1,\"status\":\"OK\"}"));
  responseHTTPGet();
  blip();
}

void closeGate()
{
  memset(jsonData, 0, sizeof jsonData);
  lngData = sprintf_P(jsonData, PSTR("{\"command\":\"porton\",\"action\":2,\"status\":\"OK\"}"));
  responseHTTPGet();
  blip();
}

void responseHTTPGet()
{
  int lng;
  memset(serialData, 0, sizeof serialData);
  lng = sprintf_P(serialData, PSTR("HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: %d \r\n"
                                   "Connection: close\r\n\r\n%s"),
                  lngData, jsonData);
  sprintf_P(wifiCmd, PSTR("AT+CIPSEND=%c,%d"), linkId, lng);
  WIFI.flush(); 
  WIFI.println(wifiCmd);
  waitESPCmdEnd();

  WIFI.flush();  
  WIFI.println(serialData);
  waitESPCmdEnd();

  memset(wifiCmd, 0, sizeof wifiCmd);
  sprintf_P(wifiCmd, PSTR("AT+CIPCLOSE=%c"), linkId);
  WIFI.flush();
  WIFI.println(wifiCmd);
  ///waitESPCmdEnd();
}

void readSensors()
{
  char cHum[8] = "0";
  char cTemp[8] = "0";

  char bmpTemp[8];
  char bmpPres[8];

  float fData = 0;

  //Leo Humedad
  fData = dht.readHumidity();
  dtostrf(fData, 5, 2, cHum);
  //Leo Temperatura
  fData = dht.readTemperature();
  dtostrf(fData, 5, 2, cTemp);

  //Leo Presión Temperatura
  fData = BMP.readTemperature();
  dtostrf(fData, 5, 2, bmpTemp);

  int32_t ibmpPres = BMP.readPressure();
  dtostrf(ibmpPres, 7, 0, bmpPres);

  memset(jsonData, 0, sizeof jsonData);
  lngData = sprintf_P(jsonData, PSTR("{\"dhtTemp\": %s,\"dhtHum\": %s,"
                                     "\"bmpTemp\": %s,\"bmpPress\": %s,"
                                     "\"rainVal\": %d,\"stationId\":\"M0001\"}"),
                      cTemp, cHum, bmpTemp, bmpPres,rainVal);
}
void startWifiServer()
{
  WIFI.println(F("AT+CIPSERVER=0"));
  waitESPCmdEnd();
  WIFI.println(F("AT+CIPMUX=1"));
  waitESPCmdEnd();
  WIFI.println(F("AT+CIPSERVER=1,80"));
  waitESPCmdEnd();
}

bool waitStringWifi(String str, bool echo)
{
  startMillis=millis();
  cadena= "";
  while(cadena.indexOf(str)==-1 && cadena.indexOf("ERROR")==-1)
  {
    if((millis()-startMillis)>=5000) return false;
    while(WIFI.available()>0)
    {
      char character = WIFI.read();
      cadena.concat(character); 
      if (character == '\n')
        {
          if(echo) Serial.print(cadena);
          if(cadena.indexOf(str)!=-1) return true;
          else
          cadena="";
        }
    }
  }
  
}


int waitESPCmdEnd()
{
  startMillis=millis();
  char data[15];
  while (strstr(data, "OK") == NULL 
        && strstr(data, ">") == NULL 
        && strstr(data, "SEND OK") == NULL 
        && strstr(data, "CLOSED") == NULL)
  {
    if((millis()-startMillis)>=5000) return false;
    memset(data, 0, sizeof data);
    if (WIFI.available() > 0)
    {
      WIFI.readBytesUntil('\n', data, 15);
    }
  }
  
}

int waitESPGotIp()
{
  memset(serialData, 0, sizeof serialData);
  while (strstr(serialData, "WIFI GOT IP") == NULL)
  {
    memset(serialData, 0, sizeof serialData);
    if (WIFI.available() > 0)
    {
      WIFI.readBytesUntil('\n', serialData, 100);
    }
  }
}

void sendToShift()
{

  digitalWrite(sftLatch, LOW);
  shiftOut(sftData, sftClock, MSBFIRST, shftVal);
  digitalWrite(sftLatch, HIGH);
  
}

void blip()
{
  tone(speakerPin, 3440);
  delay(80);
  noTone(speakerPin);
  tone(speakerPin, 3040);
  delay(90);
  noTone(speakerPin);
  tone(speakerPin, 3550);
  delay(80);
  noTone(speakerPin);
}
