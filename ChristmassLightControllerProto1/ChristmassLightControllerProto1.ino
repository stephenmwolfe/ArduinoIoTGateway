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

//Program Speed Factors
const int TWINKLE_SF = 5; //Lower numbers make twinkles faster
const int FADES_SF = 3;  //Lower numbers make fades faster

//Limits
const int  MAX_GROUP = 1;
const int  MAX_PROGRAM = 1;
const int  MAX_SPEED = 100;
const int  MAX_LED_VALUE = 255;

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
  
  private: LED * currLed;
  private: LED * groupLeds[2];

  private: int seqSpeed;
  private: int program;
  private: int dir;
  private: float counter;
  private: int seqStep;

  public: LEDGroup(LED * inLedA, LED * inLedB, int inSpeed, int inProgram){
    ledA = inLedA;
    ledB = inLedB;

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
        counter = 0;
        seqStep = 0;
        currLed = ledA;
      }
       
    }
    
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

  public: groupStep() {

    switch(program) {
      case RAND_TWINKLE: 
        randTwinkleStep();
        break;
      case FADES: 
        fadesStep();
        break;
    }
    
  }

  private: randTwinkleStep() {

    counter = ((float)seqSpeed/(float)TWINKLE_SF) + counter;

    if(counter >= MAX_SPEED) {
      switch(seqStep) {
        case 0: //Step One - pick random LED, turn it on
          currLed = groupLeds[random(2)];
          currLed->setValue(MAX_LED_VALUE);
          seqStep++;
          break;
        case 1: //Step two - turn LED off
          currLed->setValue(0);
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
      case 0: //Step One - fade in first LED
        ledA->fade(fadeIncrement);

        if(currLed->getValue() == MAX_LED_VALUE) {
          seqStep++;
        }
        
        break;
      case 1: //Step Two - fade in second LED
        ledB->fade(fadeIncrement);

        if(ledB->getValue() == MAX_LED_VALUE) {
          seqStep++;
        }
        
        break;
      case 2: //Step Three - fade out first LED
        ledA->fade(fadeIncrement * -1);

        if(currLed->getValue() == 0) {
          seqStep++;
        }
        
        break;
      case 3: //Step Four - fade out second LED
        ledB->fade(fadeIncrement * -1);

        if(ledB->getValue() == 0) {
          seqStep = 0;
        }
        
        break;
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

void randTwinkle(int Speed) {

  int rn = random(4);

  allLeds[rn]->setValue(255);
  delay(Speed);
  allLeds[rn]->setValue(0);
  delay(Speed);
      
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
