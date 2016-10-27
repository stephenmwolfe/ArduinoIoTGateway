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
const int DISABLED = 0;
const int RAND_TWINKLE = 1;
const int FADES = 2;
const int ALT_TWINKLE = 3;
const int CROSS_FADE = 4;
const int CONSTANT = 5;

//Program Speed Factors
const int TWINKLE_SF = 5; //Lower numbers make twinkles faster
const int FADES_SF = 3;  //Lower numbers make fades faster

//Limits
const int  MAX_GROUP = 1;
const int  MAX_PROGRAM = 5;
const int  MAX_SPEED = 100;
const int  MAX_LED_VALUE = 255;
const int  MIN_LED_VALUE = 0;
const int  MAX_LED_PER_GROUP = 1;

int logLevel = 0;
String message;

/***************************************/

void logMessage(int level, String message) {
    if(level <= logLevel) {
      Serial.println(message);
    }
}

struct LED {

  private: int value;
  private: int id;
  private: AF_DCMotor * led;

  public: LED(AF_DCMotor * inLed, int inId){
    led = inLed;
    value = MIN_LED_VALUE;
    id = inId;
  }

  public: setValue(int inValue) {

    message = "LED ";
    message.concat(id); 
    message.concat(" inValue: ");
    message.concat(inValue);
    logMessage(9,message);
    
    if(inValue >= MIN_LED_VALUE && inValue <= MAX_LED_VALUE) {
      value = inValue;
      led->setSpeed(value);
    }
    
    if (value > MIN_LED_VALUE) {
      led->run(FORWARD);
    }
    else {
      led->run(RELEASE);
    }
  }

  public: int getValue() {
    return value;
  }

  public: int getId() {
    return id;
  }

  public: fade(int increment) {

    int targetValue = value + increment;

    if(targetValue > MAX_LED_VALUE) {
      targetValue = MAX_LED_VALUE;
    }
    else if (targetValue < MIN_LED_VALUE) {
      targetValue = MIN_LED_VALUE;
    }

    setValue(targetValue);
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

    message = "Curr LED: ";
    message.concat(currLed); 
    logMessage(7,message);
    
    nextLed = currLed + 1;
    
    if(nextLed > MAX_LED_PER_GROUP) {
      nextLed = 0;
    }
    else if(nextLed < 0) {
      nextLed = MAX_LED_PER_GROUP;
    }
    
    message = "Next LED: ";
    message.concat(nextLed); 
    logMessage(7,message);
    
    prevLed = currLed - 1;
    
    if(prevLed > MAX_LED_PER_GROUP) {
      prevLed = 0;
    }
    
    else if(prevLed < 0) {
      prevLed = MAX_LED_PER_GROUP;
    }

    message = "Prev LED: ";
    message.concat(prevLed); 
    logMessage(7,message); 
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
      case CONSTANT: 
        constantStep();
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
          groupLeds[currLed]->setValue(MIN_LED_VALUE);
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
        
        if(groupLeds[currLed]->getValue() <= MIN_LED_VALUE) {
          
          if(currLed == MAX_LED_PER_GROUP) {
            seqStep = 0; //All LEDs off, start again
          }
          incrementCurrLed(1);      
        }
        
        break;
    }

    message = "FadeIncrement: ";
    message.concat(fadeIncrement); 
    logMessage(9,message);

    message = "LED Value: ";
    message.concat(groupLeds[currLed]->getValue()); 
    logMessage(9,message);
  }

  private: altTwinkleStep() {
    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {

      groupLeds[currLed]->setValue(MIN_LED_VALUE);
      incrementCurrLed(1);
      groupLeds[currLed]->setValue(MAX_LED_VALUE);
      
      counter = 0;
    }
  }

  private: crossFadeStep() {

    float fadeIncrement = (float)seqSpeed/(float)FADES_SF;

    if(fadeIncrement < 1) {
      counter++;
      if(counter > FADES_SF) {
        counter = 0;
        fadeIncrement = 1;
      }
    }

    groupLeds[currLed]->fade(fadeIncrement);
    groupLeds[prevLed]->fade(fadeIncrement * -1);

    message = "FadeIncrement: ";
    message.concat(fadeIncrement); 
    logMessage(9,message);

    message = "LED Value: ";
    message.concat(groupLeds[currLed]->getValue()); 
    logMessage(9,message);

    if(groupLeds[currLed]->getValue() == MAX_LED_VALUE) {
      incrementCurrLed(1);      
    }

  }

  private: constantStep() {

    int value = seqSpeed * ((float)seqSpeed/(float)(MAX_LED_VALUE - MIN_LED_VALUE));
    value = value + MIN_LED_VALUE;

    for(int i = 0; i <= MAX_LED_PER_GROUP; i++) {
      if(groupLeds[i]->getValue() != value) {
        groupLeds[i]->setValue(value);
      }
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
  
  zero = new LED(&group1a,0);
  one = new LED(&group1b,1);
  two = new LED(&group2a,2);
  three = new LED(&group2b,3);

  allLeds[0] = zero;
  allLeds[1] = one;
  allLeds[2] = two;
  allLeds[3] = three;

  group0 = new LEDGroup(zero,one,5,RAND_TWINKLE);
  group1 = new LEDGroup(two,three,5,RAND_TWINKLE);

  allGroups[0] = group0;
  allGroups[1] = group1;


  zero->setValue(MAX_LED_VALUE);
  one->setValue(MAX_LED_VALUE);
  two->setValue(MAX_LED_VALUE);
  three->setValue(MAX_LED_VALUE);

  
  delay(1000);

  zero->setValue(MIN_LED_VALUE);
  one->setValue(MIN_LED_VALUE);
  two->setValue(MIN_LED_VALUE);
  three->setValue(MIN_LED_VALUE);
  
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
    //Serial.println("comdata: " + comdata);
    logMessage(5,"comdata: " + comdata);
    
    message = "buffSize: ";
    message.concat(buffSize);
    logMessage(5,message);

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(comdata);

    if (!root.success()) {
      logMessage(1,"parseObject() failed");
      return;
    }

    //check for log level commands

    if(root["logLvl"].is<int>()) {
      logLevel = root["logLvl"];
      
      message = "Log Level Set To: ";
      message.concat(logLevel); 
      logMessage(1,message);
    }

    //check for group commands
    if(root["group"].is<int>()) {
      comGroup = root["group"];

      message = "Group: ";
      message.concat(comGroup); 
      logMessage(3,message);

      if(comGroup >=0 and comGroup <=MAX_GROUP) {
        
        //Program Update
        if(root["program"].is<int>()) {
          comProgram = root["program"];

          message = "Program: ";
          message.concat(comProgram); 
          logMessage(3,message);

          if(comProgram >=0 and comProgram <=MAX_PROGRAM) {
            allGroups[comGroup]->setProgram(comProgram);
          }
          else {
            logMessage(1,"Invalid Program");
          }
        }

        //Speed Update
        if(root["speed"].is<int>()) {
          comSpeed = root["speed"];

          message = "Speed: ";
          message.concat(comSpeed); 
          logMessage(3,message);

          if(comSpeed >=0 and comSpeed <=MAX_SPEED) {
            allGroups[comGroup]->setSpeed(comSpeed);
          }
          else {
            logMessage(1,"Invalid Speed");
          }
        }
        
      }
      else {
          logMessage(1,"Invalid Group");
      }

      
    }

  }

  logMessage(9,"Cycle Start");
  
  group0->groupStep(); //Move LED Group 0 along
  group1->groupStep(); //Move LED Group 1 along

  Serial.flush();
  
}
/****************************************/


