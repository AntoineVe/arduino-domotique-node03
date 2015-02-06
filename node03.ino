// Node03
// Chambre 1
// Temperature, humidite (DHT22)
// Gestion du chauffage
// Consommation electrique chauffage
// Consommation electrique prises de courant

// WebServer
#include <UIPEthernet.h>
byte mac[] = { 
  0x00, 0x00, 0x00, 0x01, 0x01, 0x03 };
byte ip[] = { 
  192, 168, 1, 23 };
EthernetServer server(80);
boolean reading = false;
#define BUFSIZ 100

// DHT
#include "dht.h"
dht DHT;
#define DHTPIN 3

int DHT22_VCC = 2;
int DHT22_GND = 4;
int DHT22_SIG = 3;

// Relais
int RELAIS_VCC = 5; 
int RELAIS_SIG1 = 6;
int RELAIS_VCC2 = A1;
int RELAIS_GND = 7;

// Conso chauffage
int AMP_CH_SIG = A0;
int AMP_CH_VCC = 8;
int AMP_CH_GND = 9;
int OFFSET_CHAUFF = 2494;

// Calcul de la consommation avec les ACS712 (5A ou 20A)
int adc_zero;   //autoadjusted relative digital zero
const unsigned long sampleTime = 100000UL;                           // sample over 100ms, it is an exact number of cycles for both 50Hz and 60Hz mains
const unsigned long numSamples = 250UL;                               // choose the number of samples to divide sampleTime exactly, but low enough for the ADC to keep up
const unsigned long sampleInterval = sampleTime/numSamples;  // the sampling interval, must be longer than then ADC conversion time
float readCurrent(int PIN, int AMP)
{
  float COEF;
  unsigned long currentAcc = 0;
  unsigned int count = 0;
  unsigned long prevMicros = micros() - sampleInterval ;
  while (count < numSamples)
  {
    if (micros() - prevMicros >= sampleInterval)
    {
      int adc_raw = analogRead(PIN) - adc_zero;
      currentAcc += (unsigned long)(adc_raw * adc_raw);
      ++count;
      prevMicros += sampleInterval;
    }
  }
  if (AMP == 20) {
    COEF = 50.00;
  }
  else if (AMP == 5) {
    COEF = 27.027;
  }
  float rms = sqrt((float)currentAcc/(float)numSamples) * (COEF / 1024.0);
  return rms;
}
int determineVQ(int PIN) {
  Serial.print("estimating avg. quiscent voltage:");
  long VQ = 0;
  //read 5000 samples to stabilise value
  for (int i=0; i<5000; i++) {
    VQ += analogRead(PIN);
    delay(1);//depends on sampling (on filter capacitor), can be 1/80000 (80kHz) max.
  }
  VQ /= 5000;
  Serial.print(map(VQ, 0, 1023, 0, 5000));
  Serial.println(" mV");
  return int(VQ);
}

void setup() {
  pinMode(DHT22_VCC, OUTPUT);
  pinMode(DHT22_GND, OUTPUT);
  pinMode(DHT22_SIG, INPUT);
  digitalWrite(DHT22_VCC, HIGH);
  digitalWrite(DHT22_GND, LOW);
  pinMode(RELAIS_VCC, OUTPUT);
  pinMode(RELAIS_GND, OUTPUT);
  pinMode(RELAIS_SIG1, OUTPUT);
  pinMode(RELAIS_VCC2, OUTPUT);
  digitalWrite(RELAIS_VCC, HIGH);
  digitalWrite(RELAIS_VCC2, HIGH);
  digitalWrite(RELAIS_GND, LOW);
  pinMode(AMP_CH_SIG, INPUT);
  pinMode(AMP_CH_VCC, OUTPUT);
  pinMode(AMP_CH_GND, OUTPUT);
  digitalWrite(AMP_CH_VCC, HIGH);
  digitalWrite(AMP_CH_GND, LOW);
  adc_zero = determineVQ(AMP_CH_SIG);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
}

void loop() {
  char clientline[BUFSIZ];
  int index = 0;
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          // continue to read more data!
          continue;
        }
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *command;
          command = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;    
          String comm = String(command[0]);
          comm += String(command[1]);
          if (comm == "C1" ) {
            digitalWrite(RELAIS_SIG1, LOW);
          }
          if (comm == "C0") {
            digitalWrite(RELAIS_SIG1, HIGH);
          }
        }

        if (c == '\n' && currentLineIsBlank) {
          int chk = DHT.read22(DHT22_SIG);
          float h = DHT.humidity;
          float t = DHT.temperature;
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/xml");
          client.println();
          client.println("<?xml version=\"1.0\"?>");
          // Chambre 1
          client.println("<node>");
          client.println("\t<name>Chambre 1</name>");
          client.println("\t<sensor>");
          client.println("\t\t<name>Temperature</name>");
          client.print("\t\t<value>");
          client.print(t);
          client.println("</value>");
          client.println("\t\t<type>DHT22</type>");
          client.println("\t</sensor>");
          client.println("\t<sensor>");
          client.println("\t\t<name>Humidite</name>");
          client.print("\t\t<value>");
          client.print(h);
          client.println("</value>");
          client.println("\t\t<type>DHT22</type>");
          client.println("\t</sensor>");
          client.println("</node>");
          // Chauffage Chambre 1
          client.println("<node>");
          client.println("\t<name>Chauffage chambre 1</name>");
          client.println("\t<sensor>");
          client.println("\t\t<name>Watts</name>");
          client.print("\t\t<value>");
          client.print(readCurrent(AMP_CH_SIG, 5));
          client.println("</value>");
          client.println("\t\t<type>ACS712-05B</type>");
          client.println("\t</sensor>");
          client.println("</node>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }

}
