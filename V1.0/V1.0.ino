#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>

//sfhtreg
#define sftData 12
#define sftClock 11
#define sftEnable 10

#define speakerPin 8

SoftwareSerial WIFI(3, 2); // RX | TX
Adafruit_BMP085 BMP;

//Sensor humedad - Temperatura

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

char linkId;
char serialData[256]; //entrada - salida puerto serie
int lngData;
char jsonData[150];

byte shftVal = 0;

void setup()
{
  pinMode(sftEnable, OUTPUT);   // make the data pin an output
  digitalWrite(sftEnable, LOW); // apago shift
  pinMode(sftClock, OUTPUT);    // make the clock pin an output
  pinMode(sftData, OUTPUT);     // make the data pin an output

  pinMode(speakerPin, OUTPUT);

  Serial.begin(9600);

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
  char cmd[20];
  memset(serialData, 0, sizeof serialData);
  lng = sprintf_P(serialData, PSTR("HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: %d \r\n"
                                   "Connection: close\r\n\r\n%s"),
                  lngData, jsonData);
  sprintf_P(cmd, PSTR("AT+CIPSEND=%c,%d"), linkId, lng);
  WIFI.println(cmd);
  waitESPCmdEnd();
  WIFI.println(serialData);
  waitESPCmdEnd();
  memset(cmd, 0, sizeof cmd);
  sprintf_P(cmd, PSTR("AT+CIPCLOSE=%c"), linkId);
  WIFI.println(cmd);
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
                                     "\"bmpTemp\": %s,\"bmpPress\": %s}"),
                      cTemp, cHum, bmpTemp, bmpPres);
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

int waitESPCmdEnd()
{
  char data[10];
  memset(data, 0, sizeof data);
  while (strstr(data, "OK") == NULL && strstr(data, ">") == NULL && strstr(data, "SEND OK") == NULL)
  {
    memset(data, 0, sizeof data);
    if (WIFI.available() > 0)
    {
      WIFI.readBytesUntil('\n', data, 10);
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
  digitalWrite(sftEnable, LOW);
  digitalWrite(sftEnable, HIGH);
  shiftOut(sftData, sftClock, MSBFIRST, shftVal);
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
