#include "SIM900.h"
#include <SoftwareSerial.h>
#include "HttpClientSim900.h"
#include <MemoryFree.h>
//#include "inetGSM.h"
//#include "sms.h"
//#include "call.h"

//To change pins for Software Serial, use the two lines in GSM.cpp.

//GSM Shield for Arduino
//www.open-electronics.org
//this code is based on the example of Arduino Labs.

//Simple sketch to start a connection as client.


HttpClientSim900 http;
//InetGSM inet;
//CallGSM call;
//SMSGSM sms;

char msg[1024];
int numdata;
char inSerial[50];
int i=0;
boolean started=false;

void setup()
{
     //Serial connection.
     Serial.begin(9600);
     Serial.println("GSM Shield testing.");
     //Start configuration of shield with baudrate.
     //For http uses is raccomanded to use 4800 or slower.
     if (gsm.begin(9600)) {
          Serial.println("\nstatus=READY");
          started=true;
     } else Serial.println("\nstatus=IDLE");

     if(started) {
          //GPRS attach, put in order APN, username and password.
          //If no needed auth let them blank.
          
          if (http.begin(1, "internet.telenor.se"))
               Serial.println("status=ATTACHED");
          else Serial.println("status=ERROR");
          delay(1000);
          http.saveBearer(1);
          
          //http.begin(1);
          //Read IP address.
          //gsm.SimpleWriteln("AT+CIFSR");
          //delay(5000);
          //Read until serial buffer is empty.
          //gsm.WhileSimpleRead();

          int response;
          int pos;
          
          response = http.httpGet("logger.brobild.se/conf/12345678.conf", true);
          //http.httpRequest("logger.brobild.se", "/conf/12345678.conf");
          //http.httpAction(0);
          gsm.WhileSimpleRead();
          //numdata=inet.httpGET("www.google.com", 80, "/", msg, 50);
          //Print the results.
          Serial.println("\nResponse:");
          Serial.println(http.getResponseCode());
          Serial.println("\nLength:");
          Serial.println(http.getResponseLength());
          pos = 0;
          gsm.WhileSimpleRead();
          while (http.readRow(msg, 100) > 0){
            Serial.println(msg);
          }
          http.terminate();
          

          strcpy(msg, "api_key=UXFRV6J4E10IZR22");
          response = http.httpPost("api.thingspeak.com/talkbacks/525/commands/execute", msg);
          gsm.WhileSimpleRead();
          //numdata=inet.httpGET("www.google.com", 80, "/", msg, 50);
          //Print the results.
          Serial.println("\nResponse:");
          Serial.println(http.getResponseCode());
          Serial.println("\nLength:");
          Serial.println(http.getResponseLength());
          pos = 0;
          gsm.WhileSimpleRead();
          while (http.readResponse(msg, pos, 100) > 0){
            Serial.println(msg);
            pos += 100;
          }
          http.terminate();

          http.close();
          //Serial.println("\nData received:");
          //Serial.println(msg);
     }
};

void loop()
{
     //Read for new byte on serial hardware,
     //and write them on NewSoftSerial.
     serialhwread();
     //Read for new byte on NewSoftSerial.
     serialswread();
};

void serialhwread()
{
     i=0;
     if (Serial.available() > 0) {
          while (Serial.available() > 0) {
               inSerial[i]=(Serial.read());
               delay(10);
               i++;
          }

          inSerial[i]='\0';
          if(!strcmp(inSerial,"/END")) {
               Serial.println("_");
               inSerial[0]=0x1a;
               inSerial[1]='\0';
               gsm.SimpleWriteln(inSerial);
          }
          //Send a saved AT command using serial port.
          if(!strcmp(inSerial,"TEST")) {
               Serial.println("SIGNAL QUALITY");
               gsm.SimpleWriteln("AT+CSQ");
          }
          //Read last message saved.
          if(!strcmp(inSerial,"MSG")) {
               Serial.println(msg);
          } else {
               Serial.println(inSerial);
               gsm.SimpleWriteln(inSerial);
          }
          inSerial[0]='\0';
     }
}

void serialswread()
{
     gsm.SimpleRead();
}
