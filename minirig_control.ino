/*
   minirig_control

   This sketch provides for serial communication and control over components of the pellet dispenser version of the minirig.

   Lee Lovejoy
   ll2833@columbia.edu
   July, 2016
*/

#define DEBUG_MODE 0

/*
   Baud rate
*/
#define BAUDRATE 115200

/*
   Analog Input Pins
   A0--Joystick
   A1--Infrared range finder
*/
#define JOYSTICK_ANALOG_INPUT_PIN 0
#define RANGEFINDER_ANALOG_INPUT_PIN 1

/*
   Digital Input Pins
   D2--Infrared beam break for pellet dispenser (set to an interrupt)
   D3--Infrared beam break for reward retrieval (set to an interrupt)
*/
#define PELLET_SENSOR_DIGITAL_INPUT_PIN 2
#define REWARD_RETRIEVAL_DIGITAL_INPUT_PIN 3

/*
   Digital Output Pins
   D12--Dispense pellet
   D13--Debug pin
*/
#define PELLET_DISPENSE_DIGITAL_OUTPUT_PIN 12
#define DEBUG_DIGITAL_OUTPUT_PIN 13

/*
   Command codes
*/
#define SAMPLE_JOYSTICK 10
#define SAMPLE_RANGEFINDER 11
#define WRITE_PELLET_DELAYTIME 20
#define WRITE_PELLET_ATTEMPTS 21
#define WRITE_RETRIEVAL_DELAYTIME 22
#define DISPENSE_PELLET 30
#define WRITE_CHECK_CONNECTION 99

/*
   variables for tracking pellet dispensing
*/
long pelletDispenseAttemptedTime = 0;
volatile long pelletDetectedTime = 0;
const long maxDispenseAttempts = 12;
long pelletDispenseAttemptCount = 0;
const long maxDispenseWaitTime = 300;
volatile boolean pelletDispenseAttempted = false;
volatile boolean pelletDetected = false;
volatile boolean dispensePelletFailed = false;

/*
   variables for tracking reward retrieval
*/
volatile long rewardRetrievedTime;
volatile boolean rewardRetrieved = false;

/*
   variables for controlling execution
*/
int currentCommand;
boolean readyForNextCommand = true;



/*
   setup

   Run on Arduino startup
*/
void setup() {
  //  Open up serial connection
  Serial.begin(BAUDRATE);

  //  Open digital input pins and set as interrupts
  pinMode(PELLET_SENSOR_DIGITAL_INPUT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PELLET_SENSOR_DIGITAL_INPUT_PIN), detectPelletDispense, FALLING);

  pinMode(REWARD_RETRIEVAL_DIGITAL_INPUT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(REWARD_RETRIEVAL_DIGITAL_INPUT_PIN), detectRewardRetrieval, FALLING);

  //  Set digital output pin
  pinMode(PELLET_DISPENSE_DIGITAL_OUTPUT_PIN, OUTPUT);
  digitalWrite(PELLET_DISPENSE_DIGITAL_OUTPUT_PIN, LOW);
}

/*
   loop

   Run on each iteration
*/
void loop() {

  //  Start by checking serial buffer for new data if we are ready for a new serial command
  if (readyForNextCommand && Serial.available() > 0) {
    currentCommand = (int) Serial.read();
    readyForNextCommand = false;
  }

  //  Execute pending serial command.
  if (!readyForNextCommand) {

    //  Switch operation based on command and set to ready for next command once complete
    switch (currentCommand) {

      //  Sample joystick position and write to serial
      case (SAMPLE_JOYSTICK): {
          sendAnalog(JOYSTICK_ANALOG_INPUT_PIN);
          readyForNextCommand = true;
          break;
        }

      //  Sample infrared range finder and write to serial
      case (SAMPLE_RANGEFINDER): {
          sendAnalog(RANGEFINDER_ANALOG_INPUT_PIN);
          readyForNextCommand = true;
          break;
        }

      // Write pellet delay time to serial, but only if the pellet has been detected
      // or we are certain that dispense failed; otherwise go through another cycle.
      case (WRITE_PELLET_DELAYTIME): {
          if (pelletDetected) {
            sendBytes(pelletDetectedTime - pelletDispenseAttemptedTime);
            readyForNextCommand = true;
          } else if (dispensePelletFailed) {
            sendBytes(666);
            readyForNextCommand = true;
          }
          break;
        }

      // Write number of pellet dispense attempts to serial; if no pellet dropped and still attempting return 0
      case (WRITE_PELLET_ATTEMPTS): {
          if (pelletDetected || dispensePelletFailed) {
            sendBytes(pelletDispenseAttemptCount);
          } else {
            sendBytes(0);
          }
          readyForNextCommand = true;
          break;
        }

      // Write retrieval delay time to serial, but only if the pellet has been detected
      // or we are certain that dispense failed; otherwise go through another cycle.
      case (WRITE_RETRIEVAL_DELAYTIME): {
          if (rewardRetrieved) {
            sendBytes(rewardRetrievedTime - pelletDetectedTime);
            readyForNextCommand = true;
          }
          break;
        }

      // Dispense pellet
      case (DISPENSE_PELLET): {
          pelletDispenseAttemptCount = 0;
          pelletDispenseAttempted = true;
          pelletDetected = false;
          dispensePelletFailed = false;
          rewardRetrieved = false;
          dispensePellet();
          readyForNextCommand = true;
          break;
        }

      // Check connection
      case (WRITE_CHECK_CONNECTION): {
          sendBytes(1);
          readyForNextCommand = true;
          break;
        }

      // Command not recognized
      default: {
          readyForNextCommand = true;
          break;
        }
    }
  }

  // Execute the following on every iteration

  // If we have attempted to dispense a pellet, then check and see if we've done it in time.
  if (pelletDispenseAttempted) {
    if (!pelletDetected && millis() - pelletDispenseAttemptedTime > maxDispenseWaitTime && pelletDispenseAttemptCount <= maxDispenseAttempts) {
      // We've attempted to dispense a pellet but we have not yet detected it and the max time has elapsed.
      // Presumably we failed to dispense so try to dispense again.
      dispensePellet();
    } else if (pelletDispenseAttemptCount == maxDispenseAttempts && !pelletDetected) {
      dispensePelletFailed = true;
    }
  }
}


/*
   dispensePellet

   Void function to dispense a pellet.  Sets logical flags, increments attempt count, and records time.
*/
void dispensePellet() {
  digitalWrite(PELLET_DISPENSE_DIGITAL_OUTPUT_PIN, HIGH);
  digitalWrite(PELLET_DISPENSE_DIGITAL_OUTPUT_PIN, LOW);

  if (DEBUG_MODE) digitalWrite(DEBUG_DIGITAL_OUTPUT_PIN, HIGH);
  pelletDispenseAttemptCount += 1;
  pelletDispenseAttemptedTime =  millis();
}

/* INTERRUPT SERVICE ROUTINE
   detectPelletDispense

   Void function that caputres time at which pellet detected on descent
*/
void detectPelletDispense() {
  if (DEBUG_MODE) digitalWrite(DEBUG_DIGITAL_OUTPUT_PIN, LOW);

  pelletDetectedTime = millis();
  pelletDetected = true;
  pelletDispenseAttempted = false;
}

/*  INTERRUPT SERVICE ROUTINE
   detectRewardRetrieval

   Void function that captures time at which monkey retrieves reward
*/
void detectRewardRetrieval() {
  rewardRetrievedTime = millis();
  rewardRetrieved = true;
}

/*
   sendAnalog

   Void function to sample data from specified analong pin and write it to the serial buffer
*/
void sendAnalog(int analogPin) {
  byte analogBytes[2];
  long2bytes(analogBytes, analogRead(analogPin));
  Serial.write(analogBytes, 2);
}

/*
   sendBytes

   Void function to convert an integer or long into a 2 byte word and write to werial buffer
*/
void sendBytes(long val) {
  byte outputBytes[2];
  long2bytes(outputBytes, val);
  Serial.write(outputBytes, 2);
}

/*
   long2bytes

   void function to convert a long into a 2 byte word
*/
void long2bytes(byte buffer[], long val) {
  buffer[0] = val & 255;
  buffer[1] = (val >> 8) & 255;
}

/*
   bytes2long

   Void function to convert a 2 byte word into a long.
*/
long bytes2long(byte buffer[]) {
  long val = buffer[0] + (buffer[1] << 8);
  return val;
}

