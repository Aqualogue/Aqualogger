
#include "configure.h"

void blink(){
  for(int i=0;i<10;i++){
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(300);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(300);  
  }
  
}

/***************DHT***************/
#ifdef use_dht

  dht DHT;
  int at;  // AMBIENT TEMPERATURE VAR
  int h;  // HUMIDITY VAR
  
  
  String getDHT(){
    String out;
      Serial.println("Humidity and temperature\n\n");
      h = (int) DHT.humidity;
      at = (int) DHT.temperature;
  
      DHT.read11(dht_dpin);
  
      Serial.print("Current humidity = ");
      Serial.print(DHT.humidity);
      Serial.print("%  ");
      Serial.print("temperature = ");
      h = (int) DHT.humidity;
      at = (int) DHT.temperature;
      Serial.print(at);
      Serial.println("C  ");
    out = "temp=";
    out+=at;
    out+="&humidity=";
    out+=h;
    
    return out;
    
  }
#endif
/***********************************************/






/***********pH***************************/


#ifdef use_ph

struct CalData{
  float ph7;
  float ph4;
};

  /******CALIBRATION************/

  extern CalData caldata;
  //#define Offset 25.381818181818183            //deviation compensate
  //#define slope -5.454545454545454
  
  //#define ph7_voltage  3.36
  //#define ph4_voltage  3.92
  //#define slope -3/(ph4_voltage - ph7_voltage);  

  /*****************************/
  
  #define pHArrayLength  1000    //times of collection
  
  double avergearray(int* arr, int number){
    int i;
    int max,min;
    double avg;
    long amount=0;
    if(number<=0){
      Serial.println("Error number for the array to avraging!/n");
      return 0;
    }
    if(number<5){   //less than 5, calculated directly statistics
      for(i=0;i<number;i++){
        amount+=arr[i];
      }
      avg = amount/number;
      return avg;
    }else{
      if(arr[0]<arr[1]){
        min = arr[0];max=arr[1];
      }
      else{
        min=arr[1];max=arr[0];
      }
      for(i=2;i<number;i++){
        if(arr[i]<min){
          amount+=min;        //arr<min
          min=arr[i];
        }else {
          if(arr[i]>max){
            amount+=max;    //arr>max
            max=arr[i];
          }else{
            amount+=arr[i]; //min<=arr<=max
          }
        }//if
      }//for
      avg = (double)amount/(number-2);
    }//if
    return avg;
  }
  
  float getVoltage(){
    int pHArray[pHArrayLength];   //Store the average value of the sensor feedback
    int pHArrayIndex = 0;    
    float voltage;
    
    for(int i=0; i<pHArrayLength; i++){
        delay(5);
        pHArray[pHArrayIndex++] = analogRead(pHSensorPin); 
        }  
    
    voltage = avergearray(pHArray, pHArrayLength)*5.0/1024;
    return voltage;
  }
  
  void cal7(){
    float ph7v=getVoltage();
    caldata.ph7 = ph7v;
    EEPROM.put(0,caldata);
    blink();
  }
  
  void cal4(){
    float voltage=getVoltage();
    caldata.ph4 = voltage;
    Serial.println(voltage,4);
    EEPROM.put(0,caldata);
    blink();
  }  
  
  String getpH(){
    String out;
    float slope = -3/(caldata.ph4 - caldata.ph7);

    float voltage = getVoltage();
    float pHValue = 7 + (voltage-caldata.ph7)*slope;
    
    Serial.println();
    Serial.print("Voltage:");
    Serial.print(voltage,4);
    Serial.print("    pH value: ");
    Serial.println(pHValue,2);  out = "pH=";
  
    out = "ph=";
    char str_ph[6]; //needed for println to print float
    dtostrf(pHValue, 4, 2, str_ph); //might move to its own function
    
    out += str_ph;
    return out;
  }

#endif
/*****************************************/

/**********ONEWIRE TEMP*******************/

#ifdef use_onewire
  float watertemp;
  
  OneWire oneWire(ONE_WIRE_BUS); 
  
  
  DallasTemperature sensors(&oneWire);
  
  String getwatertemp(int id) 
  { 
   String out;
   // call sensors.requestTemperatures() to issue a global temperature 
   // request to all devices on the bus 
  
   sensors.requestTemperatures(); // Send the command to get temperature readings 
  
  
   watertemp = sensors.getTempCByIndex(id);
   Serial.print("Water Temperature is: "); 
   Serial.print(sensors.getTempCByIndex(id)); // Why "byIndex"?  
     // You can have more than one DS18B20 on the same bus.  
     // 0 refers to the first IC on the wire 
     
   
  out = "water_temp=";
  out += watertemp;
  return out;
   
  } 
#endif
/******************************************/
 
//##################################################################################
//-----------  Do not Replace R1 with a resistor lower than 300 ohms    ------------
//##################################################################################
 
 
int R1= 1000;
int Ra=25; //Resistance of powering Pins 

  //** Adding Digital Pin Resistance to [25 ohm] to the static Resistor *********//
  // Consule Read-Me for Why, or just accept it as true
  
//*************Compensating for temperature ************************************//
//The value below will change depending on what chemical solution we are measuring
//0.019 is generaly considered the standard for plant nutrients [google "Temperature compensation EC" for more info
float TemperatureCoef = 0.019; //this changes depending on what chemical we are measuring
 
 
 
//********************** Cell Constant For Ec Measurements *********************//
//Mine was around 2.9 with plugs being a standard size they should all be around the same
//But If you get bad readings you can use the calibration script and fluid to get a better estimate for K
float K=2.88;
  
 
float Temperature=10;
float EC=0;
float EC25 =0;
int ppm =0;
 
 
float raw= 0;
float Vin= 5;
float Vdrop= 0;
float Rc= 0;
float buffer=0; 

//******************************************* End of Setup **********************************************************************//
 
//************ This Loop Is called From Main Loop************************//
String GetEC(){
String out = "ec=";

Temperature=DHT.temperature;
  R1=(R1+Ra);// Taking into acount Powering Pin Resitance

//************Estimates Resistance of Liquid ****************//
digitalWrite(ECPower,HIGH);
raw= analogRead(ECPin);
raw= analogRead(ECPin);// This is not a mistake, First reading will be low beause if charged a capacitor
digitalWrite(ECPower,LOW);
 
//***************** Converts to EC **************************//
Vdrop= (Vin*raw)/1024.0;
Rc=(Vdrop*R1)/(Vin-Vdrop);
Rc=Rc-Ra; //acounting for Digital Pin Resitance
EC = 1000/(Rc*K);
 
//*************Compensating For Temperaure********************//

EC25  =  EC/ (1+ TemperatureCoef*(Temperature-25.0));

char str_EC25[10]; //needed for println to print float
dtostrf(EC25, 4, 10, str_EC25); //might move to its own function
out += str_EC25;
return out;

;}

