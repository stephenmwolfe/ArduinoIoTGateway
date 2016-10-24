#include <ArduinoJson.h>

/***************************************/
//const int yellow = 9;  // yellow LED attached to pin 9
//const int green = 10;  // green LED attached to pin 10
//const int ledClear = 6;  // clear LED attached to pin 6
//const int red = 5;  // red LED attached to pin 5

struct LED {
  public: int pin;
  public: String command;
  public: int dir;
  public: int currVal;

  public: LED(int inPin, String inCommand, int inDir, int inCurrVal){
    pin = inPin;
    command = inCommand;
    dir = inDir;
    currVal = inCurrVal;
  }

  public: LED(){
  }
};

LED leds[4];
LED *yellow;
LED *green;
LED *clr;
LED *red;

//LED leds[4] = 

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
  
  yellow = new LED(9,"twinkle",1,0);
  green = new LED(10,"twinkle",1,0);
  clr = new LED(6,"twinkle",1,0);
  red = new LED(5,"twinkle",1,0);
  
  pinMode(yellow->pin,OUTPUT);  
  pinMode(green->pin,OUTPUT);  
  pinMode(clr->pin,OUTPUT);  
  pinMode(red->pin,OUTPUT);  
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
  
  digitalWrite(red,LOW);
  
  if(command == "twinkle") {
    randTwinkle(twinkleSpeed);
  }
  else if(command == "fades") {
    fades(fadeSpeed);
  }
  else {
        digitalWrite(red,HIGH);
  }
  Serial.flush();
  
}
/****************************************/

void randTwinkle(int Speed) {

  int rn = random(4);
  int led = clr->pin;

  switch(rn) {
    case 0:
      led = green->pin;
      break;
    case 1:
      led = red->pin;
      break;
    case 2:
      led = yellow->pin;
      break;
    case 3:
      led = clr->pin;
      break;
  }

  twinkle(led,Speed);
}

void fades(int fadeSpeed) {

  //fade red in
  fade(in,red->pin,fadeSpeed);
  fade(in,clr->pin,fadeSpeed);
  fade(in,yellow->pin,fadeSpeed);
  fade(in,green->pin,fadeSpeed);
  delay(1000);  //wait for a second
  //fade red out
  fade(out,red->pin,fadeSpeed);
  fade(out,clr->pin,fadeSpeed);
  fade(out,yellow->pin,fadeSpeed);
  fade(out,green->pin,fadeSpeed);
  delay(1000);  //wait for a second
  
}

void twinkle (int ledPin, int Speed) {
  digitalWrite(ledPin,HIGH);
  delay(Speed);
  digitalWrite(ledPin,LOW);
  delay(Speed);
}

//The function to drive motor rotate clockwise
void fade(int dir, int ledPin, int Speed)
{
  if(dir == 1) {
    // fade in from min to max in increments of 5 points:
    for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += Speed) {
      // sets the value (range from 0 to 255):
      analogWrite(ledPin, fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
      digitalWrite(ledPin,HIGH);
    }
  }
  else {
    for (int fadeValue = 255 ; fadeValue >=0 ; fadeValue -= Speed) {
      // sets the value (range from 0 to 255)
      analogWrite(ledPin, fadeValue);
      // wait for 30 milliseconds to see the dimming effect
      delay(30);
      digitalWrite(ledPin,LOW);
    }
  }
}



/****************************************/
