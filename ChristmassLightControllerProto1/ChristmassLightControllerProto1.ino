#include <ArduinoJson.h>

#include <AFMotor.h>

AF_DCMotor group1a(1);
AF_DCMotor group1b(2);
AF_DCMotor group2a(3);
AF_DCMotor group2b(4);

//Directions
const int IN = 1;
const int OUT = 0;

//Programs
const int RAND_TWINKLE = 0;
const int FADES = 1;
const int ALT_TWINKLE = 2;
const int CROSS_FADE = 3;

//Program Speed Factors
const int TWINKLE_SF = 5; //Lower numbers make twinkles faster
const int FADES_SF = 3;  //Lower numbers make fades faster

//Limits
const int  MAX_GROUP = 1;
const int  MAX_PROGRAM = 3;
const int  MAX_SPEED = 100;
const int  MAX_LED_VALUE = 255;
const int  MAX_LED_PER_GROUP = 1;

/***************************************/

struct LED {

  private: int value;
  private: AF_DCMotor * led;

  public: LED(AF_DCMotor * inLed){
    led = inLed;
  }

  public: setValue(int inValue) {
    if(inValue >= 0 && inValue <= MAX_LED_VALUE) {
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

  public: fade(int increment) {

    int targetValue = value + increment;

    if(targetValue > MAX_LED_VALUE) {
      targetValue = MAX_LED_VALUE;
    }
    else if (targetValue < 0) {
      targetValue = 0;
    }

    setValue(targetValue);
  }

  public: int getValue() {
    return value;
  }
  
};

struct LEDGroup {

  private: LED * ledA;
  private: LED * ledB;
  
  private: int currLed;
  private: int prevLed;
  private: int nextLed;
  
  private: LED * groupLeds[2];

  private: int seqSpeed;
  private: int program;
  private: int dir;
  private: float counter;
  private: int seqStep;

  public: LEDGroup(LED * inLedA, LED * inLedB, int inSpeed, int inProgram){
    ledA = inLedA;
    ledB = inLedB;
    setCurrLed(0);
    
    groupLeds[0] = ledA;
    groupLeds[1] = ledB;

    setSpeed(inSpeed);
    setProgram(inProgram);
  }

  public: setSpeed(int inSpeed) {
    if(inSpeed >= 0 and inSpeed <= MAX_SPEED) {
      seqSpeed = inSpeed;
    }
  }

  public: int getSpeed() {
    return seqSpeed;
  }

  public: setProgram(int inProgram) {
    if(inProgram >= 0 && inProgram <=MAX_PROGRAM) {
      if(inProgram != program) {
        program = inProgram;
        reset();
      }
       
    }
    
  }

  public: reset() {
    for (int i=0; i <= MAX_LED_PER_GROUP; i++) {
      groupLeds[i]->setValue(0);
    }
    counter = 0;
    seqStep = 0;
    setCurrLed(0);
  }

  public: int getProgram() {
    return program;
  }

  public: setDirection(int inDir) {
    if(inDir >= 0 && inDir <=1) {
      dir = inDir; 
    }
  }

  public: int getDirection() {
    return dir;
  }

  private: incrementCurrLed(int increment) {
    setCurrLed(currLed + increment);
  }

  private: setCurrLed(int in) {
    currLed = in;
    if(currLed > MAX_LED_PER_GROUP) {
      currLed = 0;
    }
    else if(currLed < 0) {
      currLed = MAX_LED_PER_GROUP;
    }

    //Serial.print("Curr LED: ");
    //Serial.println(currLed);
    
    nextLed = currLed + 1;
    
    if(nextLed > MAX_LED_PER_GROUP) {
      nextLed = 0;
    }
    else if(nextLed < 0) {
      nextLed = MAX_LED_PER_GROUP;
    }
    
    //Serial.print("Next LED: ");
    //Serial.println(nextLed);
    
    prevLed = currLed - 1;
    
    if(prevLed > MAX_LED_PER_GROUP) {
      prevLed = 0;
    }
    
    else if(prevLed < 0) {
      prevLed = MAX_LED_PER_GROUP;
    }

    //Serial.print("Prev LED: ");
    //Serial.println(prevLed);    
  }

  public: groupStep() {

    switch(program) {
      case RAND_TWINKLE: 
        randTwinkleStep();
        break;
      case FADES: 
        fadesStep();
        break;
      case ALT_TWINKLE: 
        altTwinkleStep();
        break;
      case CROSS_FADE: 
        crossFadeStep();
      break;
        
    }
    
  }

  private: randTwinkleStep() {

    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {
      switch(seqStep) {
        case 0: //Step One - pick random LED, turn it on
          currLed = random(2);
          groupLeds[currLed]->setValue(MAX_LED_VALUE);
          seqStep++;
          break;
        case 1: //Step two - turn LED off
          groupLeds[currLed]->setValue(0);
          seqStep = 0;
          break;
      }
      counter = 0;
    }
  }

  private: fadesStep() {

    float fadeIncrement = seqSpeed/FADES_SF;

    if(fadeIncrement < 1) {
      counter++;
      if(counter > FADES_SF) {
        counter = 0;
        fadeIncrement = 1;
      }
    }

    switch(seqStep) {
      case 0: //Step One - fade in LEDs
        groupLeds[currLed]->fade(fadeIncrement);

        if(groupLeds[currLed]->getValue() == MAX_LED_VALUE) {
          
          if(currLed == MAX_LED_PER_GROUP) {
            seqStep++; //All LEDs on, go to next step
          }
          incrementCurrLed(1);      
        }
        
        break;

      case 1: //Step Three - fade out first LED
        groupLeds[currLed]->fade(fadeIncrement * -1);
        
        if(groupLeds[currLed]->getValue() == 0) {
          
          if(currLed == MAX_LED_PER_GROUP) {
            seqStep = 0; //All LEDs off, start again
          }
          incrementCurrLed(1);      
        }
        
        break;
    }
  }

  private: altTwinkleStep() {
    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {

      groupLeds[currLed]->setValue(0);
      incrementCurrLed(1);
      groupLeds[currLed]->setValue(MAX_LED_VALUE);
      
      counter = 0;
    }
  }

  private: crossFadeStep() {

    float fadeIncrement = seqSpeed/FADES_SF;

    if(fadeIncrement < 1) {
      counter++;
      if(counter > FADES_SF) {
        counter = 0;
        fadeIncrement = 1;
      }
    }

    groupLeds[currLed]->fade(fadeIncrement);
    groupLeds[prevLed]->fade(fadeIncrement * -1);

    if(groupLeds[currLed]->getValue() == MAX_LED_VALUE) {
      incrementCurrLed(1);      
    }
  }
  
};

LED *zero;
LED *one;
LED *two;
LED *three;

LED *allLeds[4];

LEDGroup *group0;
LEDGroup *group1;

LEDGroup *allGroups[2];

String comdata = "twinkle";
int    comProgram = 0;
int    comSpeed = 10;
int    comGroup = 0;


boolean noCommand = false;
int lastLength = 0;
int buffSize = 0;
int fadeSpeed = 5;

/***************************************/
void setup()
{
  
  zero = new LED(&group1a);
  one = new LED(&group1b);
  two = new LED(&group2a);
  three = new LED(&group2b);

  allLeds[0] = zero;
  allLeds[1] = one;
  allLeds[2] = two;
  allLeds[3] = three;

  group0 = new LEDGroup(zero,one,5,RAND_TWINKLE);
  group1 = new LEDGroup(two,three,5,RAND_TWINKLE);

  allGroups[0] = group0;
  allGroups[1] = group1;


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
  delay(10);
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

    StaticJsonBuffer<200> jsonBuffer;
    Serial.print("jsonTmp: ");
    Serial.println(jsonTmp);
    JsonObject& root = jsonBuffer.parseObject(jsonTmp);

    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    if(root["group"].is<int>()) {
      comGroup = root["group"];
      Serial.print("Group: ");
      Serial.println(comGroup);

      if(comGroup >=0 and comGroup <=MAX_GROUP) {
        
        //Program Update
        if(root["program"].is<int>()) {
          comProgram = root["program"];
          Serial.print("Program: ");
          Serial.println(comProgram);

          if(comProgram >=0 and comProgram <=MAX_PROGRAM) {
            allGroups[comGroup]->setProgram(comProgram);
          }
          else {
            Serial.println("Invalid Program");
          }
        }

        //Speed Update
        if(root["speed"].is<int>()) {
          comSpeed = root["speed"];
          Serial.print("Speed: ");
          Serial.println(comSpeed);

          if(comSpeed >=0 and comSpeed <=MAX_SPEED) {
            allGroups[comGroup]->setSpeed(comSpeed);
          }
          else {
            Serial.println("Invalid Speed");
          }
        }
        
      }
      else {
          Serial.println("Invalid Group");
      }

      
    }
    
    const char* tmp = root["method"];
    Serial.print("Command: ");
    Serial.println(tmp);


  }
  
  group0->groupStep(); //Move LED Group 0 along
  group1->groupStep(); //Move LED Group 1 along

  Serial.flush();
  
}
/****************************************/


