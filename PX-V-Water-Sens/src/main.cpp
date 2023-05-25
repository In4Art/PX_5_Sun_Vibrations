#include <Arduino.h>

#include <CapacitiveSensor.h>

#define ACTIVE_SENSORS 6
#define NUM_SAMPLES 60

CapacitiveSensor   lqSens1 = CapacitiveSensor(2,3); 
CapacitiveSensor   lqSens2 = CapacitiveSensor(4,5);  
CapacitiveSensor   lqSens3 = CapacitiveSensor(6,7);  
CapacitiveSensor   lqSens4 = CapacitiveSensor(8,9);  
CapacitiveSensor   lqSens5 = CapacitiveSensor(10,11);  
CapacitiveSensor   lqSens6 = CapacitiveSensor(12,13);   

uint8_t txData = 0XFF;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);

  pinMode(A1, INPUT_PULLUP); // used for liquid level sensor

  
  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  long sens[6];
  sens[0] =  lqSens1.capacitiveSensor(NUM_SAMPLES);


  sens[1] =  lqSens2.capacitiveSensor(NUM_SAMPLES);


  sens[2] =  lqSens3.capacitiveSensor(NUM_SAMPLES);


  sens[3] =  lqSens4.capacitiveSensor(NUM_SAMPLES);


  sens[4] =  lqSens5.capacitiveSensor(NUM_SAMPLES);


  sens[5] =  lqSens6.capacitiveSensor(NUM_SAMPLES);


  txData = 0xFF;
  for(uint8_t i = 0; i < ACTIVE_SENSORS; i++){
    if(sens[i] > 475){ //previous threshold was 300 which seemed too low for reliable operation
      txData &= ~(1 << (i + 2));
    }
  }

  if(digitalRead(A1) == LOW ){
    txData &= ~(1 << 0);
  }

  Serial.write(txData);

  delay(200);
}