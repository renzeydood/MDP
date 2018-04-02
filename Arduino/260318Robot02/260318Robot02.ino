#include <Streaming.h>
#include <SharpIR.h>
#include <DualVNH5019MotorShield.h>
#include "communication.h"
#include "RingBuffer.h"
#include "Settings.h"
#include <Math.h>

#ifdef DEBUG
#define D if(1)
#else
#define D if(0)                   //Change this: 1 = Debug mode, 0 = Disable debug prints
#endif

void setup() {
  Serial.begin(115200);
  RingBuffer_init(&usbBufferIn);
  D Serial.println("Robot: Hello World!");
  md.init();

  //Initialise Motor Encoder Pins, digitalWrite high to enable PullUp Resistors
  pinMode(m1EncA, INPUT);
  pinMode(m1EncB, INPUT);
  pinMode(m2EncA, INPUT);
  pinMode(m2EncB, INPUT);

  //Innitializes the Motor Encoders for Interrupts
  pciSetup(m1EncA);
  pciSetup(m1EncB);
  pciSetup(m2EncA);
  pciSetup(m2EncB);

  delay(2000);
  D Serial.println("Initializations Done");
  /*scanFORWARD(&irFrontReadings[0]);
  scanRIGHT(&irRightReadings[0]);
  scanLEFT();
 */
  calibrationPhase1();
  delay(2000);
  //calibrationPhase2();
  //calibrateFRONTV2();
}


void loop() {
  if (commands[0] != 0) {
    stringCommands();
  }
  else {
    commWithRPI();
  }
}



//------------Functions for robot movements------------//
void goFORWARD(int distance) {
  long lastTime = micros();
  int setSpdR = 400;//400;                //Original: 300
  int setSpdL = 400;//400;                //Original: 300
  int colCounter = 0;
  resetMCounters();
  lastError = 0;
  totalErrors = 0;
  lastTicks[0] = 0;
  lastTicks[1] = 0;
  int i = 50;
  while (i < 401) {
    if (micros() - lastTime > 50) {
      md.setSpeeds(i, i + 10);
      i++;
      lastTime = micros();
    }
  }
  lastTime = millis();
  delay(50);

  if (distance <= 1192) {
    while (mCounter[0] < distance && mCounter[1] < distance) {
      if (millis() - lastTime > 100) {
        PIDControl(&setSpdR, &setSpdL, 150, 7, 30, 0); //By block
        lastTime = millis();
        md.setSpeeds(setSpdR, setSpdL);
        //Collision check starts here;
        /*
          if (colCounter % CrashChkPeriod == 0) {
          if (checkFRONT()) {
            break;
          }
          }*/
        colCounter++;
      }
    }
  } else {
    while (mCounter[0] < distance - 445 && mCounter[1] < distance - 445) {
      if (millis() - lastTime > 100) {
        PIDControl(&setSpdR, &setSpdL, 110, 5, 15, 0); //Long distance
        lastTime = millis();
        md.setSpeeds(setSpdR, setSpdL);
      }
    }
    i = 0;
    lastTime = micros();
    while (mCounter[0] < distance && mCounter[1] < distance) {
      if (micros() - lastTime > 50) {
        md.setSpeeds(setSpdR - i, setSpdL - i);
        i++;
        if (i > 100)
          i = 100;
        lastTime = micros();
        //Collision check starts here
        /*
          if (colCounter % CrashChkPeriod == 0) {
          if (checkFRONT()) {
            break;
          }
          }*/
        colCounter++;
      }
    }
  }

  md.setBrakes(400, 400);
  resetMCounters();
  delay(100);
}

void goRIGHT(int angle) {
  int setSpdR = -300;              //Right motor
  int setSpdL = 306;              //Left motor
  long lastTime = millis();
  lastError = 0;
  totalErrors = 0;
  lastTicks[0] = 0;
  lastTicks[1] = 0;
  resetMCounters();

  md.setSpeeds(setSpdR, setSpdL);
  delay(50);
  while (mCounter[0] < angle - turnRightTicks - 200 && mCounter[1] < angle - turnRightTicks - 200) {
    if (millis() - lastTime > 100) {
      PIDControl(&setSpdR, &setSpdL, 150, 6, 15, 1);
      lastTime = millis();
      md.setSpeeds(setSpdR, setSpdL);
    }
  }
  int i = 0;
  lastTime = micros();
  while (mCounter[0] < angle - turnRightTicks && mCounter[1] < angle - turnRightTicks) {
    if (micros() - lastTime > 50) {
      md.setSpeeds(setSpdR + i, setSpdL - i);
      i++;
      if (i > 100)
        i = 100;
      lastTime = micros();
    }
  }
  md.setBrakes(400, 400);
}

void goLEFT(int angle) {
  int setSpdR = 300;              //Right motor
  int setSpdL = -315;              //Left motor
  long lastTime = millis();
  lastError = 0;
  totalErrors = 0;
  lastTicks[0] = 0;
  lastTicks[1] = 0;
  resetMCounters();

  md.setSpeeds(setSpdR, setSpdL);
  delay(50);

  while (mCounter[0] < angle - turnLeftTicks - 200 && mCounter[1] < angle - turnLeftTicks - 200) {
    if (millis() - lastTime > 100) {
      PIDControl(&setSpdR, &setSpdL, 150, 6, 15, -1);
      lastTime = millis();
      md.setSpeeds(setSpdR, setSpdL);
    }
  }
  int i = 0;
  lastTime = micros();
  while (mCounter[0] < angle - turnLeftTicks && mCounter[1] < angle - turnLeftTicks) {
    if (micros() - lastTime > 50) {
      md.setSpeeds(setSpdR - i, setSpdL + i);
      i++;
      if (i > 100)
        i = 100;
      lastTime = micros();
    }
  }
  md.setBrakes(400, 400);
}

//Direction(dr): -1 = left, 0 = straight, 1 = right
void PIDControl(int *setSpdR, int *setSpdL, int kP, int kI, int kD, int dr) {
  int adjustment;
  int error = (mCounter[1] - lastTicks[1]) - (mCounter[0] - lastTicks[0]);            //0 = right motor, 1 = left motor, lesser tick time mean faster
  int errorRate = error - lastError;
  lastError = error;
  lastTicks[0] = mCounter[0];
  lastTicks[1] = mCounter[1];
  totalErrors += 2;
  // totalErrors += error             ;                                           //Add up total number of errors (for Ki)
  if (error != 0) {                                                           //if error exists
    adjustment = ((kP * error) - (kI * totalErrors) + (kD * errorRate)) / 100;
    // adjustment = ((kP * error) + (kI * totalErrors) + (kD * errorRate)) / 100;
    if (dr == 1 || dr == -1) {
      *setSpdR += -adjustment * dr;
      *setSpdL -= adjustment * dr;
    }
    else {
      *setSpdR += adjustment;
      *setSpdL -= adjustment;
    }
  }
}

void calibrateRIGHT() {
  scanRIGHT(&irRightReadings[0]);
  int turnTicks = 0;
  while (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
    resetMCounters();

    turnTicks = (irRightReadings[0] - irRightReadings[1]) * 2;

    //Tick Reduction
    if ((abs(irRightReadings[0] - irRightReadings[1]) == 10) && abs(turnTicks) > 20) {
      turnTicks -= 1;
    }

    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-150, 150);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(150, -150);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanRIGHT(&irRightReadings[0]);
  }
}

void calibrateFRONT() {
  scanFORWARD(&irFrontReadings[0]);
  int turnTicks = 0;
  while (abs(irFrontReadings[2] - 100) > 5 && abs(irFrontReadings[0] - 100) > 5) {
    resetMCounters();
    turnTicks = (irFrontReadings[0] - 100) * 2; // old multiplier is 20
    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(200, 200);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-200, -200);
      }
    }

    //Serial.println("irfrontreadings[0] V1");
    //Serial.println(irFrontReadings[0]);
    md.setBrakes(400, 400);
    delay(100);
    scanFORWARD(&irFrontReadings[0]);
  }
}

void calibrateCORNER() {
  calibrateRIGHT();
  delay(100);
  calibrateFRONTV2();
  delay(100);
  goRIGHT(angleToTicks(90));
  delay(100);
  calibrateFRONT();
  delay(100);
  goLEFT(angleToTicks(90));
  delay(100);
  calibrateRIGHT();
}

void calibrateRIGHTV2() {
  scanRIGHT(&irRightReadings[0]);
  int turnTicks = 0;
  //  Serial << "start" << endl ;
  while (abs(irRightReadings[0] % 100 - irRightReadings[1] % 100) > 5 && abs(irRightReadings[0] - irRightReadings[1]) < 200) {
    resetMCounters();
    int Fdist = irRightReadings[0];
    int Bdist = irRightReadings[1];
    if (Fdist / 100 == Bdist / 100) {
      if (Fdist < Bdist) {
        Bdist = Bdist + 99 - Fdist % 100;
        Fdist = Fdist + 99 - Fdist % 100;
      }
      else {
        Fdist = Fdist + 99 - Bdist % 100;
        Bdist = Bdist + 99 - Bdist % 100;
      }
    }
    else if (abs(Fdist / 100 - Bdist / 100) > 1) {
      int cnt = 10 + 100 * (abs(Fdist / 100 - Bdist / 100) - 1);
      if (Fdist % 100 > Bdist % 100) {
        Bdist = Bdist + (cnt - Fdist % 100);
        Fdist = Fdist + (cnt - Fdist % 100);
      }
      else {
        Fdist = Fdist + (cnt - Bdist % 100);
        Bdist = Bdist + (cnt - Bdist % 100);
      }
    }
    turnTicks = (Fdist % 100 - Bdist % 100) * 2;
    //Tick Reduction
    if (abs((Fdist - Bdist) % 100) == 1 && abs(turnTicks) > 20) {
      turnTicks -= 1;
    }

    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-150, 150);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(150, -150);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanRIGHT(&irRightReadings[0]);
  }
}

bool checkFRONT() {
  scanFORWARD(&irFrontReadings[0]);
  int turnTicks = 0;
  resetMCounters();
  while (min(irFrontReadings[0], irFrontReadings[2]) < 50 || irFrontReadings[1] < 40) {
    turnTicks = max((100 - min(irFrontReadings[0], irFrontReadings[2])), (90 - irFrontReadings[1])) * 2;
    while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
      md.setSpeeds(-200, -200);
    }
    md.setBrakes(400, 400);
    delay(100);
  }
  if (turnTicks > 0) {
    return true;
  }
  return false;
}

void fwdCorrection() {
  //  int start = mCounter[0];
  //  int correction = 1.2 * (start - mCounter[1]);
  //
  //  while (mCounter[0] - start < correction){
  //    md.setM1Speed(-300);
  //  }
  //  md.setBrakes(400, 400);
  //  resetMCounters();
  md.setM1Speed(-395);
  delay(7);
  md.setBrakes(400, 400);
  resetMCounters();
}


int angleToTicks(long angle) {
  if (angle == 90)
    return 16800 * angle / 1000;
  else
    return (17280 * angle / 1000) - aboutTurnOffset;
}

int blockToTicks(int blocks) {
  if (blocks == 1)
    if (counter > 0)
      return (ticksToMove - forwardOffsetTicks) * blocks;
    else
      return (1183 - forwardOffsetTicks) * blocks;
  else
    return 1192 * blocks;
}



//------------Experimental------------//
void calibrateFRONTV2() {
  int zTicks = 0;
  scanFORWARD(&irFrontReadings[0]);
  int turnTicks = 0;
  while (abs(irFrontReadings[2] - 100) > 5 && abs(irFrontReadings[0] - 100) > 5) {
    resetMCounters();
    turnTicks = (irFrontReadings[0] - 100) * 2;
    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(200, 200);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-200, -200);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanFORWARD(&irFrontReadings[0]);
    //Serial.println("irfrontreadings[0] V2");
    //Serial.println(irFrontReadings[0]);
    zTicks += turnTicks;
  }
  if(mvmtCounter[0] == 0){
    ticksToMove = ticksToMove + (kTicks * zTicks / mvmtCounter[0]);
  }
}

void calibrationPhase1() {

  calibrateCORNER();

  delay(200);
  int cycle = 8;
  /*
  for (int i = 0; i < cycle; i++) {
    goLEFT(angleToTicks(90));
    delay(200);
  }
  calibrateLeftTurnOffset(cycle);
  delay(200);
  
  for (int i = 0; i < cycle; i++) {
    goRIGHT(angleToTicks(90));
    delay(200);
  }
  calibrateRightTurnOffset(cycle);
  delay(200);
  */

  for (int i = 0; i < cycle; i++) {
    goLEFT(angleToTicks(180));
    delay(200);
  }
  calibrateAboutTurnOffset(cycle);
  delay(200);
  
}

void calibrationPhase2() {/*
  int cycle = 8;

  goLEFT(angleToTicks(90));
  delay(200);

  calibrateRIGHT();
  delay(200);

  scanRIGHT(&irRightReadings[0]);

  for (int i = 0; i < cycle; i++) {
    goFORWARD(blockToTicks(1));
    delay(200);
  }
  calibrateOffset(3, cycle);

  goLEFT(angleToTicks(180));
  delay(200);

  for (int i = 0; i < cycle; i++) {
    goFORWARD(blockToTicks(1));
    delay(200);
  }
  
    goLEFT(angleToTicks(180));
    delay(200);

    calibrateRIGHT();
    delay(200);

    for(int i = 0; i < cycle/2; i++){
      goFORWARD(blockToTicks(1));
      delay(200);
    }

    goLEFT(angleToTicks(180));
    delay(200);

    for(int i = 0; i < cycle/2; i++){
      goFORWARD(blockToTicks(1));
      delay(200);
    }
  

  goLEFT(angleToTicks(90));
  delay(200);*/
}

// flag = 0: LEFT offset, 1: RIGHT offset, 2: ABOUT_TURN offset, 3: FORWARD pull-pull back ticks
void calibrateAboutTurnOffset(int cycle) {

  int frontRight = irRightReadings[0];
  int backRight = irRightReadings[1];

  int offset = 0;

  scanRIGHT(&irRightReadings[0]);
  int turnTicks = 0;
  while (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
    resetMCounters();

    turnTicks = (irRightReadings[0] - irRightReadings[1]) * 2;

    //Tick Reduction
    if ((abs(irRightReadings[0] - irRightReadings[1]) == 10) && abs(turnTicks) > 20) {
      turnTicks -= 1;
    }

    offset += turnTicks;

    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-150, 150);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(150, -150);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanRIGHT(&irRightReadings[0]);
  }

  delay(200);
  aboutTurnOffset = aboutTurnOffset + (offset / cycle);
}



// flag = 0: LEFT offset, 1: RIGHT offset, 2: ABOUT_TURN offset, 3: FORWARD pull-pull back ticks
void calibrateLeftTurnOffset(int cycle) {

  int frontRight = irRightReadings[0];
  int backRight = irRightReadings[1];

  int offset = 0;

  scanRIGHT(&irRightReadings[0]);
  int turnTicks = 0;
  while (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
    resetMCounters();

    turnTicks = (irRightReadings[0] - irRightReadings[1]) * 2;

    //Tick Reduction
    if ((abs(irRightReadings[0] - irRightReadings[1]) == 10) && abs(turnTicks) > 20) {
      turnTicks -= 1;
    }

    offset += turnTicks;

    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-150, 150);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(150, -150);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanRIGHT(&irRightReadings[0]);
  }

  delay(200);
  turnLeftTicks = turnLeftTicks + (offset / cycle);
}


// flag = 0: LEFT offset, 1: RIGHT offset, 2: ABOUT_TURN offset, 3: FORWARD pull-pull back ticks
void calibrateRightTurnOffset(int cycle) {

  int frontRight = irRightReadings[0];
  int backRight = irRightReadings[1];

  int offset = 0;

  scanRIGHT(&irRightReadings[0]);
  int turnTicks = 0;
  while (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
    resetMCounters();

    turnTicks = (irRightReadings[0] - irRightReadings[1]) * 2;

    //Tick Reduction
    if ((abs(irRightReadings[0] - irRightReadings[1]) == 10) && abs(turnTicks) > 20) {
      turnTicks -= 1;
    }

    offset += turnTicks;

    if (turnTicks > 0) {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(-150, 150);
      }
    }
    else {
      while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
        md.setSpeeds(150, -150);
      }
    }
    md.setBrakes(400, 400);
    delay(100);
    scanRIGHT(&irRightReadings[0]);
  }

  delay(200);
  turnRightTicks = turnRightTicks - (offset / cycle);    
}
   /*
  if(flag == 0){
    turnLeftTicks = turnLeftTicks + (offset / cycle);
  }
  delay(200);
  
  if(flag == 1){
    turnRightTicks = turnRightTicks - (offset / cycle);    
  }
  delay(200);
  */
/*
  if(flag == 3){
    double o = ((frontRight - irRightReadings[0]) + (backRight - irRightReadings[1])) / 2;
      double a = 1183 * cycle;
      double angleDeg = atan(o / a) * 180 / 3.141593;
      int ticks = 16800 * angleDeg / 1000;
      if (angleDeg > 0) {
        pullBackTicks = pullBackTicks - ticks;
      }
      else {
        // more pull back
        pullBackTicks = pullBackTicks + ticks;
      } 
  }*/


  /*
  switch (flag) {
    // LEFT
    
    case 0:
      turnLeftTicks = turnLeftTicks + (offset / cycle);
      break;
    
    // RIGHT
    case 1:
      turnRightTicks = turnRightTicks - (offset / cycle);
      break;
    // ABOUT_TURN
    case 2:
      aboutTurnOffset = aboutTurnOffset + (offset / cycle);
      break;

      
    case 3:
      double o = ((frontRight - irRightReadings[0]) + (backRight - irRightReadings[1])) / 2;
      double a = 1183 * cycle;
      double angleDeg = atan(o / a) * 180 / 3.141593;
      int ticks = 16800 * angleDeg / 1000;
      if (angleDeg > 0) {
        pullBackTicks = pullBackTicks - ticks;
      }
      else {
        // more pull back
        pullBackTicks = pullBackTicks + ticks;
      }
      //Serial << "Pull back ticks:" << pullBackTicks;
      //pullBackTicks = pullBackTicks + ((offset - (frontRight - irRightReadings[0]) * 1.5 ) /cycle);
      break;
       
  }*/

void fwdCorrectionV2() {
  resetMCounters();
  int toMove = pullBackTicks;
  if (toMove != 0) {
    // Robot need to pull back right wheel more
    if (toMove > 0) {
      while (mCounter[0] < abs(toMove)) {
        md.setM1Speed(-300);
      }
    }
    // Robot need to move right wheel forward more
    else {
      while (mCounter[0] < abs(toMove)) {
        md.setM1Speed(300);
      }
    }
  }
  md.setBrakes(400, 400);
  delay(100);
  /*turnTicks = (irFrontReadings[0] - 100) * 2;
    if (turnTicks > 0) {
    while (mCounter[0] < abs(turnTicks) && mCounter[1] < abs(turnTicks)) {
      md.setSpeeds(200, 200);
    }
    }*/

}



//------------Functions for IR Sensors------------//
void scanFORWARD(int *pData) {
  pData[0] = lfwdIrVal.distance(); //Left
  delay(2);
  pData[1] = mfwdIrVal.distance(); // Middle
  delay(2);
  pData[2] = rfwdIrVal.distance(); //Right
  delay(2);
  D Serial << "FORWARD: <- Left: " << pData[0] << " () Mid: " << pData[1] << " -> Right: " << pData[2] << " \n" << endl;
}

void scanRIGHT(int *pData) {
  pData[0] = frgtIrVal.distance(); //Right Front
  delay(2);
  pData[1] = brgtIrVal.distance(); //Right Back
  delay(2);
  D Serial << "RIGHT: -> Right(Short): " << pData[0] << " -> Right(Long): " << pData[1] << " \n" << endl;
}

void scanLEFT() {
  irLeftReading = flftIrVal.distance();
  delay(2);
  D Serial << "LEFT: <- Left(Long): " << irLeftReading << " \n" << endl;
}



//------------Functions for Motors------------//
void resetMCounters() {
  mCounter[0] = 0;
  mCounter[1] = 0;
}

//ISR for Motor 1 (Right) Encoders
ISR(PCINT2_vect) {
  mCounter[0]++;
}

//ISR for Motor 2 (Left) Encoders
ISR(PCINT0_vect) {
  mCounter[1]++;
}

//Standard function to enable interrupts on any pins
void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}



//------------Functions for communications------------//
void commWithRPI() {
  static int calCounter = 0;
  if (Serial.available() > 0) {
    putIncomingUSBMessageToBuffer();
    int traversalIndex = 0;
    uint8_t tmpInBuffer = 0;

    if (usbBufferIn.count >= 6) {
      if (RingBuffer_get( & usbBufferIn, & tmpInBuffer, 0) == true && tmpInBuffer == '~') {
        uint8_t messageType = 0;
        if (RingBuffer_get( & usbBufferIn, & tmpInBuffer, 1) == true) {
          messageType = tmpInBuffer;
        }

        if (messageType == ARDUINO_INSTRUCTION) {
          if (5 < usbBufferIn.count) {
            if (RingBuffer_get( & usbBufferIn, & tmpInBuffer, 5) == true && tmpInBuffer == '!') {

              InstructionMessage instructMsg;

              RingBuffer_get( & usbBufferIn, & instructMsg.id, 2);
              RingBuffer_get( & usbBufferIn, & instructMsg.action, 3);
              RingBuffer_get( & usbBufferIn, & instructMsg.obstacleInFront, 4);

              alreadyReceived = true;
              yetToReceiveAck = false;
              switch (instructMsg.action) {
                case TURN_LEFT:
                  goLEFT(angleToTicks(90));
                  delay(RPIExpDelay);
                  calCounter++;
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case TURN_RIGHT:
                  goRIGHT(angleToTicks(90));
                  delay(RPIExpDelay);
                  calCounter++;
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case FORWARD:
                  goFORWARD(blockToTicks(1));
                  mvmtCounter[0]++;
                  delay(RPIExpDelay);
                  fwdCorrection();
//                  fwdCorrectionV2();
                  calCounter++;
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case CAL_CORNER:
                  calibrateCORNER();
                  mvmtCounter[0] = 0;
                  delay(RPIExpDelay);
                  calCounter = 0;
                  counter++;
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case CAL_SIDE:
                  if (calCounter >= CalPeriod) {
                    if (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
                      calibrateRIGHT();
                      if ((irRightReadings[0] <= 90 || irRightReadings[0] >= 110)) {
                        delay(100);
                        goRIGHT(angleToTicks(90));
                        delay(100);
                        calibrateFRONT();
                        delay(100);
                        goLEFT(angleToTicks(90));
                        delay(100);
                        calibrateRIGHT();
                      }
                    }
                    calCounter = 0;
                  }
                  delay(RPIExpDelay);
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case SCAN:
                  delay(RPIExpDelay);
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case START:
                  delay(RPIExpDelay);
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case STOP:
                  yetToReceiveAck = false;
                  break;

                case TURN_ABOUT:
                  goLEFT(angleToTicks(180));
                  delay(RPIExpDelay);
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case CAL_ANY:
                  if (abs(irRightReadings[0] - irRightReadings[1]) < 20 && abs(irRightReadings[0] - irRightReadings[1]) > 5)
                    calibrateRIGHTV2();
                  else
                    calibrateRIGHT();
                  delay(RPIExpDelay);
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

                case CAL_FORWARD:
                  scanFORWARD(&irFrontReadings[0]);
                  if (irRightReadings[1] > 20) {
                    goFORWARD(blockToTicks(1));
                  }
                  sendStatusUpdate();
                  incrementID();
                  alreadyReceived = false;
                  break;

              }
              RingBuffer_erase( & usbBufferIn, 6);
            } else {
              RingBuffer_pop( & usbBufferIn);
            }
          }
        }

        else if (messageType == ARDUINO_STREAM) {
          StreamMessage streamMsg;
          uint8_t payloadSize = 0;
          // may not matter
          RingBuffer_get(&usbBufferIn, &streamMsg.id, 2);
          RingBuffer_get(&usbBufferIn, &payloadSize, 3);

          uint8_t tmpInBuffer;
          if (4 + payloadSize <= usbBufferIn.count) {
            if (RingBuffer_get(&usbBufferIn, &tmpInBuffer, 4 + payloadSize) == true && tmpInBuffer == '!') {
              uint8_t tmpPayload[payloadSize] = {0};
              for (int i = 0; i < payloadSize; i++) {
                RingBuffer_get(&usbBufferIn, &(tmpPayload[i]), 4 + i);
              }
              memcpy(streamMsg.streamActions, &tmpPayload, payloadSize);

              for (int i = 0; i < payloadSize; i++) {
                int forwardCount = 0;
                uint8_t action = streamMsg.streamActions[i];

                switch (action) {
                  case FORWARD:
                    forwardCount = 1;
                    while (true) {
                      if ((i + 1) < payloadSize && streamMsg.streamActions[i + 1] == FORWARD) {
                        forwardCount++;
                        i++;
                      }
                      else {
                        break;
                      }
                    }
                    goFORWARD(blockToTicks(forwardCount));
                    delay(RPIFPDelay);
                    break;

                  case TURN_RIGHT:
                    goRIGHT(angleToTicks(90));
                    delay(RPIFPDelay);
                    break;

                  case TURN_LEFT:
                    goLEFT(angleToTicks(90));
                    delay(RPIFPDelay);
                    break;

                  case TURN_ABOUT:
                    goLEFT(angleToTicks(180));
                    delay(RPIFPDelay);
                    break;
                }
              }
              sendStatusUpdate();
              incrementID();
              alreadyReceived = false;
              RingBuffer_erase(&usbBufferIn, payloadSize + 5);
            }
          }
        }
      } else {
        RingBuffer_pop( & usbBufferIn);
      }
    }
  }

  if (millis() > timer + timeout && yetToReceiveAck) {
    resendStatusUpdate();
  }
}

void stringCommands() {
  static int calCounter = 0;
  static int x;
  switch (commands[x]) {
    case 1:
      Serial.println("Moving forward");
      goFORWARD(blockToTicks(1));
      mvmtCounter[0]++;
      fwdCorrection();
      calCounter++;
      break;

    case 2:
      Serial.println("Moving left");
      goLEFT(angleToTicks(90));
      calCounter++;
      break;

    case 3:
      Serial.println("Moving right");
      goRIGHT(angleToTicks(90));
      calCounter++;
      break;

    case 4:
      Serial.println("Calibrate Right");
      scanRIGHT(&irRightReadings[0]);
      if (calCounter >= CalPeriod) {
        if (abs(irRightReadings[0] - irRightReadings[1]) > 5 && (abs(irRightReadings[0] - irRightReadings[1]) <= 70)) {
          calibrateRIGHT();
          if ((irRightReadings[0] <= 90 || irRightReadings[0] >= 110)) {
            delay(100);
            goRIGHT(angleToTicks(90));
            calibrateFRONT();
            delay(100);
            goLEFT(angleToTicks(90));
            calibrateRIGHT();
          }
          calCounter = 0;
        }
      }
      break;

    case 5:
      Serial.println("Doing Full Scan");
      scanFORWARD(&irFrontReadings[0]);
      scanLEFT();
      scanRIGHT(&irRightReadings[0]);
      Serial << "Left Forward IR: " << shortIrVal(irFrontReadings[0], 3, 340, lfwdIrOS) << " blocks away, actual: " << irFrontReadings[0] << endl;
      Serial << "Mid Forward IR: " << shortIrVal(irFrontReadings[1], 3, 350, mfwdIrOS) << " blocks away, actual: " << irFrontReadings[1] << endl;
      Serial << "Right Forward IR: " << shortIrVal(irFrontReadings[2], 3, 340, rfwdIrOS) << " blocks away, actual: " << irFrontReadings[2] << endl;
      Serial << "Front Right IR: " << shortIrVal(irRightReadings[0], 3, 340, frgtIrOS) << " blocks away, actual: " << irRightReadings[0] << endl;
      Serial << "Back Right IR: " << shortIrVal(irRightReadings[1], 3, 340, brgtIrOS) << " blocks away, actual: " << irRightReadings[1] << endl;
      Serial << "Left Long IR: " << longIrVal(irLeftReading, 5, 65, flftIrOS) << " blocks away, actual: " << irLeftReading << endl;
      break;

    case 6:
      Serial.println("Calibrate At Corner");
      calibrateCORNER();
      calCounter = 0;
      break;

    case 7:
      Serial.println("About Turn");
      goLEFT(angleToTicks(180));
      calCounter++;
      break;

    case 8:
      Serial.println("Forward burst");
      goFORWARD(blockToTicks(commands[++x]));
      calCounter++;
      break;

    case 9:
      Serial.println("Calibrate At Any blocks");
      scanRIGHT(&irRightReadings[0]);
      if (abs(irRightReadings[0] - irRightReadings[1]) < 200 && abs(irRightReadings[0] - irRightReadings[1]) > 50) {
        Serial << "V2" << endl;
        calibrateRIGHTV2();
      } else {
        Serial << "Normal" << endl;
        calibrateRIGHT();
      }
      break;
  }
  delay(commandsDelay);

  if (x <= sizeof(commands) / sizeof(int)) {
    x++;
  }
}

int shortIrVal(int val, int blockThreshold, int cmThreshold, int offset) {

  int newVal = (val - offset) / 100;
  if (val < 100) {
    newVal = 1;
  }
  if (newVal >= blockThreshold || val >= cmThreshold || val <= 0) {
    newVal = 0;
  }
  return newVal;
}

int longIrVal(int val, int blockThreshold, int cmThreshold, int offset) {
  int newVal = (val - offset) / 10;
  if (val <= 18) {
    newVal = 1;
  }

 if (newVal >= blockThreshold || val >= cmThreshold) {
    newVal = 0;
  }
  return newVal;
}

void putIncomingUSBMessageToBuffer() {
  uint8_t tmpBuffer[1024] = {0}; //not allocated
  uint8_t length = 0;

  while (Serial.available()) {
    tmpBuffer[length] = Serial.read();
    length++;
  }
  if (length) {
    for (uint16_t i = 0; i < length; i++) {
      RingBuffer_push(&usbBufferIn, tmpBuffer[i]);
    }
  }

  delay(1);
}

void resendStatusUpdate() {
  decrementID();
  sendStatusUpdate();
  incrementID();
}

void sendStatusUpdate() {
  delay(200);

  scanFORWARD(&irFrontReadings[0]);
  scanLEFT();
  scanRIGHT(&irRightReadings[0]);

  // Put sensor readings here
  StatusMessage statusPayload;
  statusPayload.id = last_sent;
  statusPayload.front1 = shortIrVal(irFrontReadings[0], 3, 340, lfwdIrOS);
  statusPayload.front2 = shortIrVal(irFrontReadings[1], 3, 350, mfwdIrOS);
  statusPayload.front3 = shortIrVal(irFrontReadings[2], 3, 340, rfwdIrOS);
  statusPayload.right1 = shortIrVal(irRightReadings[0], 3, 340, frgtIrOS);
  statusPayload.right2 = shortIrVal(irRightReadings[1], 3, 340, brgtIrOS);
  statusPayload.left1 = longIrVal(irLeftReading, 5, 65, flftIrOS);
  statusPayload.reached = 1;

  // Crafts message to send
  Message msg;
  msg.type = ARDUINO_UPDATE;
  memcpy(&msg.payload, &statusPayload, 8);

  uint8_t tmpOutBuffer[64] = {0};
  tmpOutBuffer[0] = '~';
  memcpy(&tmpOutBuffer[1], &msg, 9);
  tmpOutBuffer[10] = '!';

  Serial << '~' << (int)tmpOutBuffer[1] << (int)tmpOutBuffer[2] << (int)tmpOutBuffer[3] << (int)tmpOutBuffer[4] << (int)tmpOutBuffer[5] << (int)tmpOutBuffer[6] << (int)tmpOutBuffer[7] << (int)tmpOutBuffer[8] << (int)tmpOutBuffer[9] << '!' << endl;
  Serial.flush();

  //start_timer()
  timer = millis();
  yetToReceiveAck = true;
}

void incrementID() {
  last_sent = (last_sent + 1) % 10;
}

void decrementID() {
  if (last_sent == 0) {
    last_sent = 9;
  }
  else {
    last_sent = last_sent - 1;
  }
}