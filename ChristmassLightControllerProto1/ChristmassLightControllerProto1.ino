#include <ArduinoJson.h>
#include <AFMotor.h>
#include <EEPROM.h>

//Constants *************************************** Constants

const int EEPROM_START = 50;

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
const int GROUP_FADE = 6;
const int GROUP_TWINKLE = 7;
const int RANDOM = 8;

//Program Speed Factors
const int TWINKLE_SF = 5; //Lower numbers make twinkles faster
const int FADES_SF = 3;  //Lower numbers make fades faster

//Limits
const int  MAX_GROUP = 2;
const int  MAX_PROGRAM = 8;
const int  MAX_SPEED = 100;
const int  MAX_LED_VALUE = 255;
const int  MIN_LED_VALUE = 0;
const int  MAX_LEDS_PER_GROUP = 4;
const int  MAX_LED = 3;

//Global Primatives *************************************** Global Primatives 

String comdata = "twinkle";
int    comProgram = 0;
int    comSpeed = 10;
int    comGroup = 0;
int    comLed = 0;

boolean noCommand = false;
int lastLength = 0;
int buffSize = 0;
int fadeSpeed = 5;

int logLevel = 0;
String message;

//Global Functions *************************************** Global Functions

void logMessage(int level, String message) {
    if(level <= logLevel) {
      Serial.println(message);
    }
}

//Objects  *************************************** Objects

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
  
  private: int currLed;
  private: int prevLed;
  private: int nextLed;
  
  private: LED * groupLeds[MAX_LEDS_PER_GROUP];

  private: int seqSpeed;
  private: int program;
  private: int dir;
  private: float counter;
  private: int seqStep;
  private: int maxLed;
  private: int id;

  public: LEDGroup(int inId){
    id = inId;
    setSpeed(0);
    setProgram(0);
    maxLed = -1;
  }

  public: addLed (LED * inLed) {
    if(maxLed < (MAX_LEDS_PER_GROUP - 1)) {
      maxLed++;
      groupLeds[maxLed] = inLed;
    }
  }

  public: setSpeed(int inSpeed) {
    if(inSpeed >= 0 and inSpeed <= MAX_SPEED) {
      seqSpeed = inSpeed;
    }
  }

  public: int getSpeed() {
    return seqSpeed;
  }

  public: int getId() {
    return id;
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
    for (int i=0; i <= maxLed; i++) {
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
    if(currLed > maxLed) {
      currLed = 0;
    }
    else if(currLed < 0) {
      currLed = maxLed;
    }

    message = "Curr LED: ";
    message.concat(currLed); 
    logMessage(7,message);
    
    nextLed = currLed + 1;
    
    if(nextLed > maxLed) {
      nextLed = 0;
    }
    else if(nextLed < 0) {
      nextLed = maxLed;
    }
    
    message = "Next LED: ";
    message.concat(nextLed); 
    logMessage(7,message);
    
    prevLed = currLed - 1;
    
    if(prevLed > maxLed) {
      prevLed = 0;
    }
    
    else if(prevLed < 0) {
      prevLed = maxLed;
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
      case GROUP_FADE:
        groupFadeStep();
        break;
      case GROUP_TWINKLE:
        groupTwinkleStep();
        break;
      case RANDOM:
        randomize();
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

  private: groupTwinkleStep() {

    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {
      switch(seqStep) {
        case 0: //Step One - turn all LEDs on

          for(int i = 0; i <= maxLed; i++) {
            groupLeds[i]->setValue(MAX_LED_VALUE);
          }
          
          seqStep++;
          break;
        case 1: //Step two - turn all LEDs off
          groupLeds[currLed]->setValue(MIN_LED_VALUE);

          for(int i = 0; i <= maxLed; i++) {
            groupLeds[i]->setValue(MIN_LED_VALUE);
          }
          
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
          
          if(currLed == maxLed) {
            seqStep++; //All LEDs on, go to next step
          }
          incrementCurrLed(1);      
        }
        
        break;

      case 1: //Step Three - fade out first LED
        groupLeds[currLed]->fade(fadeIncrement * -1);
        
        if(groupLeds[currLed]->getValue() <= MIN_LED_VALUE) {
          
          if(currLed == maxLed) {
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

    for(int i = 0; i <= maxLed; i++) {
      if(groupLeds[i]->getValue() != value) {
        groupLeds[i]->setValue(value);
      }
    }
  }

  private: groupFadeStep() {

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
      
        for(int i = 0; i <= maxLed; i++) {
          groupLeds[i]->fade(fadeIncrement);
        }
        
        if(groupLeds[0]->getValue() == MAX_LED_VALUE) {
          seqStep++; //All LEDs on, go to next step   
        }
        
        break;

      case 1: //Step Three - fade out first LED
        
        for(int i = 0; i <= maxLed; i++) {
          groupLeds[i]->fade(fadeIncrement *-1);
        }
        
        if(groupLeds[0]->getValue() <= MIN_LED_VALUE) {
            seqStep = 0; //All LEDs off, start again    
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

  private: randomize() {

    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {
      //set random LED to random value
      groupLeds[random(maxLed+1)]->setValue(random(MIN_LED_VALUE,MAX_LED_VALUE));
      counter = 0;
    }
  }
  
};

//Global Objects *************************************** Global Objects

LED *zero;
LED *one;
LED *two;
LED *three;

LED *allLeds[4];

LEDGroup *group0;
LEDGroup *group1;
LEDGroup *group2;

LEDGroup *allGroups[3];

AF_DCMotor group1a(1);
AF_DCMotor group1b(2);
AF_DCMotor group2a(3);
AF_DCMotor group2b(4);

//Helper Functions *************************************** Helper Functions

void save() {

  int i = EEPROM_START;
  EEPROM.update(i++,logLevel);
  EEPROM.update(i++,group0->getProgram());
  EEPROM.update(i++,group0->getSpeed());
  EEPROM.update(i++,group1->getProgram());
  EEPROM.update(i++,group1->getSpeed());
  EEPROM.update(i++,group2->getProgram());
  EEPROM.update(i++,group2->getSpeed());
  
}

void load() {

  int i = EEPROM_START;

  int j;

  j = EEPROM.read(i++);
  logLevel = j; //get saved log level before logging

  logMessage(3,"Loading Values:");
  logMessage(3,String(j));

  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group0->setProgram(j);
  
  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group0->setSpeed(j);

  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group1->setProgram(j);
  
  
  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group1->setSpeed(j);

  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group2->setProgram(j);
  
  j = EEPROM.read(i++);
  logMessage(3,String(j));
  group2->setSpeed(j);

}

//Arduino Functions *************************************** Arduino Functions

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

  group0 = new LEDGroup(0);
  group0->addLed(zero);
  group0->addLed(one);
  group0->setProgram(1);
  group0->setSpeed(10);
  
  group1 = new LEDGroup(1);
  group1->addLed(two); 
  group1->addLed(three);
  group1->setProgram(1);
  group1->setSpeed(10);

  group2 = new LEDGroup(2);
  group2->addLed(zero);
  group2->addLed(one);
  group2->addLed(two); 
  group2->addLed(three); 

  allGroups[0] = group0;
  allGroups[1] = group1;
  allGroups[2] = group2;


  zero->setValue(MAX_LED_VALUE);
  one->setValue(MAX_LED_VALUE);
  two->setValue(MAX_LED_VALUE);
  three->setValue(MAX_LED_VALUE);

  
  delay(1000);

  zero->setValue(MIN_LED_VALUE);
  one->setValue(MIN_LED_VALUE);
  two->setValue(MIN_LED_VALUE);
  three->setValue(MIN_LED_VALUE);

  delay(1000);

  Serial.begin(9600);  // start serial port at 9600 bps:

  load();
  
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

   

    if(root["logLvl"].is<int>()) {  //check for log level commands
      logLevel = root["logLvl"];
      
      message = "Log Level Set To: ";
      message.concat(logLevel); 
      logMessage(1,message);
    }
    else if(root["group"].is<int>()) { //check for group commands
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
            //If Group 2 is enabled disable Groups 0 and 1
            if(comGroup == 2 and  comProgram > DISABLED) {
              group0->setProgram(DISABLED);
              group1->setProgram(DISABLED);
            }
            //If Group 0 or 1 is enabled disable Groups 2
            if(comGroup < 2 and  comProgram > DISABLED) {
              group2->setProgram(DISABLED);
            }
            
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
    else if (root["led"].is<int>()) {
      
      comLed = root["led"];
      message = "LED: ";
      message.concat(comLed); 
      logMessage(3,message);

      if(comLed >=0 and comLed <=MAX_LED) {
        
        //Value Update
        if(root["value"].is<int>()) {
          comSpeed = root["value"];

          message = "Value: ";
          message.concat(comSpeed); 
          logMessage(3,message);

          //Disable groups the LED is in
          group2->setProgram(DISABLED);
          if(comLed < 2) {
            group0->setProgram(DISABLED);
          }
          else {
            group1->setProgram(DISABLED);
          }
  
          allLeds[comLed]->setValue(comSpeed);
        }
      }
      else {
        logMessage(1,"Invalid LED");
      }
    }
    else if (root["save"] == 1) {
      logMessage(3,"Saving");
      save(); 
    }
    else if (root["load"] == 1) {
      logMessage(3,"Loading");
      load(); 
    }
    
  }

  logMessage(9,"Cycle Start");

  for(int i = 0; i<=MAX_GROUP; i++) {
    allGroups[i]->groupStep(); //Move LED Group i along
  }

  Serial.flush();
  
}

/****************************************/


