#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h> 
#include <dht.h> 
#include <Bounce2.h>

#include "sensors.h"
#include "configure.h"

/************NETWORKING*******/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "aqualogue.herokuapp.com";    
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 160);
EthernetClient client;
/**********************************/

/******MAIN VARIABLES FOR CONTROL LOOP*****/
//long previousMillis = 0;
//unsigned long currentMillis = 0;
//long interval = 25000; // READING INTERVAL
String data;
/***************************************/

Bounce ph7debounce=Bounce();
Bounce ph4debounce=Bounce();
CalData caldata;//Calibration Voltages



void setup(){
  Ethernet.begin(mac, ip);
  Serial.begin(115200);
  delay(300);//Let system settle  
  data = "";
  
  /*******BUTTONS***********/

  
  pinMode(ph7_button, INPUT_PULLUP);
  pinMode(ph4_button, INPUT_PULLUP);
  
  pinMode(led,OUTPUT); 
  ph7debounce.attach(ph7_button);
  ph7debounce.interval(500);
  ph4debounce.attach(ph4_button);
  ph4debounce.interval(500);
  
  
  /***EC****/
  pinMode(ECPin,INPUT);
  pinMode(ECPower,OUTPUT);//Setting pin for sourcing current
  pinMode(ECGround,OUTPUT);//setting pin for sinking current
  digitalWrite(ECGround,LOW);//We can leave the ground connected permanantly
 
  /*********/
  
  
  /**********CALIBRATION**********/
  EEPROM.get(0,caldata);
  /*****************************/
  
   
  delay(5000);//Wait 5s before accessing sensors
}

void loop(){

/********************BUTTTONS*********************/
   ph7debounce.update();
   ph4debounce.update();
  
   if (ph4debounce.read()==LOW){
     Serial.println("CALIBRATING");
     Serial.println(caldata.ph4);
     delay(5);
     cal4();
  }
  
  if (ph7debounce.read()==LOW){
   Serial.println("CALIBRATING");
   Serial.println(caldata.ph7);
   delay(5);
   cal7(); 
  }
  

  /************************************/
  
  /*******COLLECTING SENSOR DATA******/
  //These will be moved into one "get_data) function, 
  //which will compile differently depending on configure.h
  delay(5);
  data = getpH(); // pH probe
  delay(5);
  data += "&"+getwatertemp(0); //submersible thermometer
  delay(5);
  data += "&"+GetEC(); //Includes ambient temp and humidity
  delay(5);
  data += "&"+getDHT(); //Includes ambient temp and humidity

  
  Serial.println(data); //mainly for debugging
  /************************************/
  
  /**********SENDING DATA TO THE SERVER*****/
  if (client.connect("aqualogue.herokuapp.com", 80)) { // REPLACE WITH YOUR SERVER ADDRESS
    Serial.println("connected");
    client.println("GET /add_data HTTP/1.1"); 
    client.println("Host: aqualogue.herokuapp.com"); // SERVER ADDRESS HERE TOO
    client.println("Content-Type: application/x-www-form-urlencoded"); 
    client.print("Content-Length: "); 
    client.println(data.length()); 
    client.println(); 
    client.print(data); 
  } 

  if (client.connected()) { 
    client.stop();  // DISCONNECT FROM THE SERVER
  }
 /*******************************************/
  
  delay(3000);
}// end loop()

