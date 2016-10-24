#include <ArduinoJson.h>

#include <AFMotor.h>

AF_DCMotor group1a(1);
AF_DCMotor group1b(2);
AF_DCMotor group2a(3);
AF_DCMotor group2b(4);

/***************************************/

struct LED {

  public: int dir;
  private: int value;
  private: AF_DCMotor * led;

  public: LED(int inDir, int inValue, AF_DCMotor * inLed){
    dir = inDir;
    value = inValue;
    led = inLed;
  }

  public: LED(){
  }

  public: setValue(int inValue) {
    if(inValue >= 0 && inValue <= 255) {
      value = inValue;
      led->setSpeed(value);
    }
    
    if (value > 0) {
      led->run(FORWARD);
    }
    else {
      led->run(RELEASE);
    }
  }
};


LED *zero;
LED *one;
LED *two;
LED *three;

LED *allLeds[4] = {zero,one,two,three};

//fade Directions
const int in = 1;
const int out = 0;

String comdata = "twinkle";
String command = "twinkle";
boolean noCommand = false;
StaticJsonBuffer<200> jsonBuffer;
int lastLength = 0;
int buffSize = 0;
int fadeSpeed = 5;
int twinkleSpeed = 100;
int twinkleDur = 100;

/***************************************/
void setup()
{
  
  zero = new LED(1,0,&group1a);
  one = new LED(1,0,&group1b);
  two = new LED(1,0,&group2a);
  three = new LED(1,0,&group2b);

  /*
  group1a.setSpeed(255);
  group1a.run(FORWARD);
  
  group1b.setSpeed(255);
  group1b.run(FORWARD);
  
  group2a.setSpeed(255);
  group2a.run(FORWARD);
  
  group2b.setSpeed(255);
  group2b.run(FORWARD);
  */

  zero->setValue(255);
  one->setValue(255);
  two->setValue(255);
  three->setValue(255);

  
  delay(1000);

  zero->setValue(0);
  one->setValue(0);
  two->setValue(0);
  three->setValue(0);
  
  Serial.begin(9600);  // start serial port at 9600 bps:
  Serial.println("Please input command:");  //print message on serial monitor
}
/****************************************/
void loop()
{
  delay(100);
  //read string from serial monitor
  if(Serial.available()>0)  // if we get a valid byte, read analog ins:
  { 
    buffSize = Serial.available();
    //Serial.println("bytes: " + Serial.available());
    comdata = "";
    while (Serial.available() > 0)  
    {        
      comdata += char(Serial.read());
      delay(2);
    }
    Serial.println("comdata: " + comdata);
    Serial.print("buffSize: ");
    Serial.println(buffSize);
    
    char jsonTmp[200];
    comdata.toCharArray(jsonTmp, 200);
    Serial.print("jsonTmp: ");
    Serial.println(jsonTmp);
    JsonObject& root = jsonBuffer.parseObject(jsonTmp);

    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }
    
    const char* tmp = root["method"];
    command = tmp;
    Serial.print("Command: ");
    Serial.println(tmp);
      if(command == "twinkle") {
        if(root["params"]["speed"] >=0 && root["params"]["speed"] <=255) {
          twinkleSpeed = root["params"]["speed"];
          Serial.print("Twinkle Speed: ");
          Serial.println(twinkleSpeed);
        }
      }
      else if(command == "fades") {
        if(root["params"]["speed"] >=0 && root["params"]["speed"] <=255) {
          fadeSpeed = root["params"]["speed"];
          Serial.print("Fade Speed: ");
          Serial.println(fadeSpeed);
        }
      }

  }
  
  if(command == "twinkle") {
    randTwinkle(twinkleSpeed);
  }
  else if(command == "fades") {
    fades(fadeSpeed);
  }

  Serial.flush();
  
}
/****************************************/

void randTwinkle(int Speed) {

  int rn = random(4);

  /*
  allLeds[rn]->setValue(255);
  delay(Speed);
  allLeds[rn]->setValue(0);
  delay(Speed);
  */
      
  
  switch(rn) {
    case 0:
      //twinkle(group1a,Speed);
      zero->setValue(255);
      delay(Speed);
      zero->setValue(0);
      delay(Speed);
      break;
    case 1:
      //twinkle(group1b,Speed);
      one->setValue(255);
      delay(Speed);
      one->setValue(0);
      delay(Speed);
      break;
    case 2:
      //twinkle(group2a,Speed);
      two->setValue(255);
      delay(Speed);
      two->setValue(0);
      delay(Speed);
      break;
    case 3:
      //twinkle(group2b,Speed);
      three->setValue(255);
      delay(Speed);
      three->setValue(0);
      delay(Speed);
      break;
  }

}

void fades(int fadeSpeed) {

  //fade in
  fade(1,zero, fadeSpeed);
  fade(1,one, fadeSpeed);
  fade(1,two, fadeSpeed);
  fade(1,three, fadeSpeed);
  
  delay(1000);  //wait for a second
  
  //fade out
  fade(0,zero, fadeSpeed);
  fade(0,one, fadeSpeed);
  fade(0,two, fadeSpeed);
  fade(0,three, fadeSpeed);
  
  delay(1000);  //wait for a second
  
}

void twinkle (AF_DCMotor led, int Speed) {
  led.setSpeed(255);
  delay(Speed);
  led.setSpeed(0);
  delay(Speed);
}

//The function to drive motor rotate clockwise
void fade(int dir, LED * inLed, int Speed)
{
  if(dir == 1) {
    // fade in from min to max in increments of 5 points:
    for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += Speed) {
      // sets the value (range from 0 to 255):
      inLed->setValue(fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
    }
    inLed->setValue(255);
  }
  else {
    for (int fadeValue = 255 ; fadeValue >=0 ; fadeValue -= Speed) {
      // sets the value (range from 0 to 255)
      inLed->setValue(fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
    }
    inLed->setValue(0);
  }
}



/****************************************/
