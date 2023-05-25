/*
 * Firmware for PX-V
 * 
 * Pinout:
 *  Servos 
 *    0: D0 -> 16
 *    1: D1 -> 5
 *    2: D2 -> 4
 *    3: D3 -> 0
 *    4: D4 -> 2
 *  Motor controller:
 *    AIN1: D5 -> 14  
 *    AIN2: D6 -> 12
 *    FAULT: D7 -> 13
 *    SLEEP: SD3 -> 10
 *  Arduino - Helper: 
 *    RX: 1 byte with data
 * 
 */
#include <Arduino.h>


#ifdef ESP8266
 #include <ESP8266WiFi.h>
#else //ESP32
 #include <WiFi.h>
#endif

#include <ModbusIP_ESP8266.h>

#include <Servo.h>

#include "Drv8833.h"
#include "creds.h"
#include "ModeControl.h"
#include "WifiControl.h"

#define PX_NUM 5
#define PX_REG 100 + (PX_NUM * 10)
#define PX_STATE_REG 110 + PX_NUM //lowest PX_NUM = 1 !!!!

enum {
  LIQUID_LOW = -1,
  PX_OK,
  PUMP_DRV_FLT,  //driver FLT pin low -> driver chip in error mode
  PUMP_ERR, //general pumping system error
  SENS_ERR //much pumping doesn't affect sensor reading, there must be a sensor error!
};

char ssid[] = SSID ;        // your network SSID (name)
char pass[] = PW;                    // your network password
WifiControl pxWifi(ssid, pass, PX_NUM);

//ModbusIP object

ModbusIP pxModbus;


#define MT_FLT 13
#define MT_SLP 10
#define MT_1 14
#define MT_2 12
#define REG_SPEED 800 //600

Drv8833 pxvPump(MT_1, MT_2, MT_SLP, MT_FLT);


#define NUM_SERVOS 5
#define SERVO_MIN_POS 1500 // 1471//90
#ifdef POSTEST
#define SERVO_MAX_POS 1471 + ((2147 - 1471) / 2)
#else
#define SERVO_MAX_POS 2400 //2147 //2347 175
#endif
#define SERVO_STATE_DIFF 10

Servo pxServos[NUM_SERVOS];
int16_t servoGoals[NUM_SERVOS];

int16_t pxServoMstrPos = SERVO_MIN_POS;
uint32_t pxServoMstrT = 0;

void detachAllServos(void);
void attachAllServos(void);

int8_t demoState = 0;
void demoCallback(uint32_t dTime, px_mode_t mode);
ModeControl pxMC(10, &demoCallback, 25000, &pxWifi);

int8_t pxState = 0;
void setState(int8_t state);
int8_t pxShakeState = 0;
#define SHAKE_TIME 250

uint32_t checkTime = 0;
void checkState(void);

int8_t liquidState = 0;
int8_t initLiquidSys(void);
void processLiquidSensors(uint8_t data);

int8_t pxSysState = PX_OK; //start out ok


void setup() {
  // put your setup code here, to run once:
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);
  Serial.begin(57600);
  delay(1000);

  pxServos[0].attach(16); // D0
  pxServos[1].attach(5); // D1
  pxServos[2].attach(4); // D2
  pxServos[3].attach(0); // D3
  pxServos[4].attach(2); //D4

  for(uint8_t i = 0; i < NUM_SERVOS; i++){
    uint16_t mid_pos = (SERVO_MAX_POS - SERVO_MIN_POS) / 2 + SERVO_MIN_POS;
    pxServos[i].writeMicroseconds(mid_pos);
    servoGoals[i] = mid_pos;
  }

  pxServoMstrT = millis();
  
  pxWifi.setPreConn(detachAllServos);
  pxWifi.setPostConn(attachAllServos);

  //time out used for waiting for a connection to occur
  //useful to change during testing to speed things up when dev-ing with no C&C
  pxWifi.setTimeOut(30000);
   
  Serial.println("Connecting to C&C...");
  int8_t res = pxWifi.init();
  if(res == -1){
    Serial.println("No C&C found, starting up in demo mode!");
  }

   //create the modbus server, and add a holding register (addHreg) and Ireg
  pxModbus.server(502);
  pxModbus.addHreg(PX_REG, 0);
  pxModbus.addIreg(PX_REG, 0);
  pxModbus.addHreg(PX_STATE_REG, PX_OK);

  //for shake state
  pxModbus.addHreg(PX_REG + 1, 0);
  pxModbus.addIreg(PX_REG + 1, 0);

  pxMC.init();//initialize modeControl
 
    int8_t lRes = 0;
    lRes = initLiquidSys();
    if(lRes == -1){

        switch(liquidState){
            case 0: 
            pxSysState = LIQUID_LOW;
            Serial.println("LIQUID LOW DETECTED");
            break;
            case 1:
            pxSysState = PUMP_ERR;
            Serial.println("PUMP ERROR DETECTED");
            pxvPump.stop(); ///make sure to stop pumping here!
            break;
            case 2:
            break;
            default:
            pxSysState = SENS_ERR;
            Serial.println("SENSOR ERROR DETECTED");
            pxvPump.stop();
            Serial.println("pumping forward 100 secs");
            pxvPump.speed(1000, FWD);
            pxvPump.start();
            delay(100000);
            pxvPump.stop();
            Serial.println("restarting system");
            //ESP.restart();
            digitalWrite(D8, HIGH);
            delay(10);
            ESP.restart();
            break;

        }
        
    }

        Serial.println("setup finished.");


}

void loop() {
    pxModbus.task();

    pxMC.run();

  //update modbus px status reg
  if(pxModbus.Hreg(PX_STATE_REG) != pxSysState){
    pxModbus.Hreg(PX_STATE_REG, pxSysState);
  }

  //this should  copy the holding reg value to ireg
  if(pxModbus.Hreg(PX_REG) != pxModbus.Ireg(PX_REG)){
    pxModbus.Ireg(PX_REG, pxModbus.Hreg(PX_REG));
  }

  if(pxState != pxModbus.Ireg(PX_REG)){
    setState((int8_t)pxModbus.Ireg(PX_REG));
  }

  if(pxModbus.Hreg(PX_REG + 1) != pxModbus.Ireg(PX_REG + 1)){
    pxModbus.Ireg(PX_REG + 1, pxModbus.Hreg(PX_REG + 1));
  }
  
  if(pxShakeState != pxModbus.Ireg(PX_REG + 1)){
    pxShakeState = pxModbus.Ireg(PX_REG + 1);
  }

  if(millis() - pxServoMstrT > SHAKE_TIME){
    pxServoMstrT = millis();
    if(pxServoMstrPos == SERVO_MAX_POS){
      pxServoMstrPos = SERVO_MIN_POS;
    }else{
      pxServoMstrPos = SERVO_MAX_POS;
    }
    for(uint8_t i = 0; i < NUM_SERVOS; i++){
      if(i < pxShakeState){
        pxServos[i].writeMicroseconds(pxServoMstrPos);
      }else{
        pxServos[i].writeMicroseconds((SERVO_MAX_POS - SERVO_MIN_POS) / 2 + SERVO_MIN_POS);
      }
    }
  }

  if(Serial.available()){
      processLiquidSensors(Serial.read());
    }

  if(pxvPump.isRunning()){

    if(pxvPump.getDirection() == FWD){
      
      if(liquidState >= pxState + 2){
        
        delay(500);
        pxvPump.stop();
        
      }
    }else{
      
      if(liquidState == pxState + 1){
       
        pxvPump.stop();
        //we must pump FWD again beyond the sensor otherwise we cannot go back another state
        //because we don't know the state of the system anymore..
        delay(100);
        pxvPump.direction(FWD);
        pxvPump.start();
      }
    }
  }else{
    if(millis() - checkTime > 5000 && liquidState <= 2){
      checkState();
      checkTime = millis();
    }
  }

  //reconnection attempt when the unit is not shaking and 'empty'
  //this ensures that demo timing isn't affected by reconnection attempts, 
  //but also that when reconnection is succesful the system is in nice start position
  if(pxWifi.getStatus() != WL_CONNECTED && pxShakeState == 0 && pxState < 1 && liquidState <= 2){
    
    
    //here we also stop the pump in case it is running
    //insted of using the preConn and posConn functions from pxWifi because it is more tidy to keep track of
    //the pump state right here.
    //
      bool pumpState;
      pumpState = pxvPump.isRunning();
      if(pumpState){
        pxvPump.stop();
      }
        pxWifi.reConn();

      if(pumpState){
        pxvPump.start();
      }
  }

}

void setState(int8_t state)
{
  if(pxSysState == PX_OK){
    //only set states when system is ok 

    if(pxState != state){
      
      pxState = state;
      if(liquidState  < state + 2){
        Serial.println("starting motor FWD");
        pxvPump.speed(REG_SPEED, FWD);
        //pxvPump.direction(FWD);
        pxvPump.start();
        //pxState = state;
      }else if(liquidState > state + 2 ){
        Serial.println("starting motor BWD");
        pxvPump.speed(REG_SPEED, BWD);
        //pxvPump.direction(BWD);
        pxvPump.start();
        //pxState = state;
      }
    }
  }

}

void checkState(void){
  if(pxSysState == PX_OK){
    if(liquidState  < pxState + 2){
        Serial.println("starting motor FWD");
        pxvPump.speed(REG_SPEED, FWD);
        //pxvPump.direction(FWD);
        pxvPump.start();
        //pxState = state;
      }else if(liquidState > pxState + 2 ){
        Serial.println("starting motor BWD");
        pxvPump.speed(REG_SPEED, BWD);
        //pxvPump.direction(BWD);
        pxvPump.start();
        //pxState = state;
      }
  }
}

int8_t initLiquidSys(void)
{
  //get a serial reading and determine if level in container is ok -> liquidState > 0
 // uint8_t liquid_sensors = 0;

  Serial.println("pumping BWD 100secs");
  pxvPump.speed(1000, BWD);
  pxvPump.start();
  delay(100000);
  pxvPump.stop();
  delay(2000);

  /* SPECIAL TEST CODE!!!*/
  /*pxvPump.speed(1000, FWD);
  pxvPump.start();
  delay(100000);
  pxvPump.stop();*/
  /*END OF SPECIAL TEST CODE*/

  Serial.println("waiting for serial data");
  while(Serial.available() == 0 ){
    ESP.wdtFeed(); //feed the watchdog so we don't reset
  }
  
  processLiquidSensors(Serial.read());

  
  if(liquidState == 0){
    pxvPump.speed(1000, BWD);
    pxvPump.start();
    delay(10000);
    pxvPump.stop();
    while(Serial.available() == 0 );
  
    processLiquidSensors(Serial.read());
    if(liquidState == 0) return -1;
    //liquid level error!
  }
  //pump liquid past 1st sensor and stop
  //pump FWD if level <= 1
  //backwards if > 1
  if(liquidState > 1 ){
    Serial.println("BWD liquidstate > 1");
    pxvPump.speed(REG_SPEED, BWD);
  }else{
    Serial.println("FWD liquidstate <= 1");
    pxvPump.speed(REG_SPEED, FWD);
  }
  pxvPump.start();

  uint32_t startTime = millis();

  bool pumping = true;

  while(pumping && (millis() - startTime) < 30000){

       if(Serial.available()){
            processLiquidSensors(Serial.read());
       }
    
      if(pxvPump.getDirection() == FWD && liquidState == 2){
              delay(500);
              pxvPump.stop();
              pumping = false;
          
      }else if(pxvPump.getDirection() == BWD && liquidState == 1){
        pxvPump.stop();
        //we must pump FWD again beyond the sensor to get to a know state
        delay(100);
        pxvPump.direction(FWD);
        pxvPump.start();
      }
    
  }

  if(pumping){
    //timeout must have kicked it out of above while loop -> something isn't right
    return -1;
    //timeout error!
  }

  if(liquidState != 2){
    return -1;
  }
  return 0;
}

void processLiquidSensors(uint8_t data)
{
  //check all 7 sensors, the eigth bit can be used for something else
  //1 level sensor
  //6 liquid gates
  //actual states are:
  //0 -> low level in container
  //1 -> ok level in container
  //2 -> 1st sensor (before first loops)
  //3 -> 2nd sensor
  //4 -> 3rd sensor
  //5 -> 4th sensor
  //6 -> 5th sensor
  //7 -> 6th sensor (end)
  //difference between pxState and liquidstate -> pxState 0 = liquidstate 2

  if(data & 0x01){
    liquidState = 1;
  }else{
    liquidState = 0;
  }
  for(int8_t i = 1; i < 8; i++){
    if(!(data & (1 << i))){
        liquidState = i;
      
    }
  }
  Serial.println(liquidState);  
}


void demoCallback(uint32_t dTime, px_mode_t mode){
  if(mode == PX_DEMO_MODE){
    if(demoState > 19){
          demoState = 0;
          
        }
        
        if(demoState > 5){
          if(demoState < 11){
            pxModbus.Hreg(PX_REG + 1, demoState - 5);
            pxMC.setInterval(10000);
          }else if(demoState < 16){
            pxModbus.Hreg(PX_REG + 1, 15 - demoState );
          }else{
            pxMC.setInterval(25000);
            pxModbus.Hreg(PX_REG, 20 - demoState);
            Serial.print("Checking PX status reg in demo mode: ");
            Serial.println(pxModbus.Hreg(PX_REG));
            Serial.print("LiquidState: ");
            Serial.println(liquidState);
          }
        }else{
          if(demoState == 0){
            //extra long mode zero interval
            pxMC.setInterval(35000);
          }
          pxModbus.Hreg(PX_REG, demoState);
          Serial.print("Checking PX status reg in demo mode: ");
          Serial.println(pxModbus.Hreg(PX_REG));
          Serial.print("LiquidState: ");
          Serial.println(liquidState);
        }
        demoState++;
  }else if(mode == PX_CC_MODE){
    demoState = 0;
      pxModbus.Hreg(PX_REG, demoState);
  }
}

void detachAllServos(){
  pxServos[0].detach(); // D0
    pxServos[1].detach();  // D1
    pxServos[2].detach();  // D2
    pxServos[3].detach();  // D3
    pxServos[4].detach();  //D4
}

void attachAllServos()
{
  pxServos[0].attach(16); // D0
    pxServos[1].attach(5);  // D1
    pxServos[2].attach(4);  // D2
    pxServos[3].attach(0);  // D3
    pxServos[4].attach(2);  //D4
}