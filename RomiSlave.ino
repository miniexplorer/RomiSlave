#include <Wire.h>
#include <Romi32U4.h>

PololuBuzzer buzzer;
Romi32U4Motors motors;
Romi32U4ButtonA buttonA;
Romi32U4ButtonB buttonB;
Romi32U4ButtonC buttonC;
Romi32U4Encoders encoders;

// Commands list
enum {
  I2C_CMD_GET_COUNTS = 0xA0,
  I2C_CMD_GET_COUNTS_AND_RESET = 0xA4,
  I2C_CMD_SET_SPEEDS = 0xA1,
  I2C_CMD_SET_LEFT_SPEED = 0xA2,
  I2C_CMD_SET_RIGHT_SPEED = 0xA3,
};

enum {
  I2C_MSG_ARGS_MAX = 32,
  I2C_RESP_LEN_MAX = 32
};

#define I2C_ADDR                 20

extern const byte supportedI2Ccmd[] = {
  0xA0,
  0xA4,
  0xA1,
  0xA2,
  0xA3
};

int argsCnt = 0;                        // how many arguments were passed with given command
int requestedCmd = 0;                   // which command was requested (if any)

byte i2cArgs[I2C_MSG_ARGS_MAX];         // array to store args received from master
int i2cArgsLen = 0;                     // how many args passed by master to given command

uint8_t i2cResponse[I2C_RESP_LEN_MAX];  // array to store response
int i2cResponseLen = 0;                 // response length

void setup()
{
  // Start i2c
  Wire.begin(I2C_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  encoders.init();
  Serial.println("Romi starting.");
}

void loop()
{
  if (requestedCmd == I2C_CMD_GET_COUNTS) {
    i2cResponseLen = 0;

    int16_t encLeftVal = 0, encRightVal = 0;
    encLeftVal = encoders.getCountsLeft();
    encRightVal = encoders.getCountsRight();

    i2cResponse[0] = highByte(encLeftVal);
    i2cResponse[1] = lowByte(encLeftVal);
    i2cResponse[2] = highByte(encRightVal);
    i2cResponse[3] = lowByte(encRightVal);
    i2cResponseLen += 4;

    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
  else if (requestedCmd == I2C_CMD_GET_COUNTS_AND_RESET) {
    i2cResponseLen = 0;

    int16_t encLeftVal = 0, encRightVal = 0;
    encLeftVal = encoders.getCountsAndResetLeft();
    encRightVal = encoders.getCountsAndResetRight();

    i2cResponse[0] = highByte(encLeftVal);
    i2cResponse[1] = lowByte(encLeftVal);
    i2cResponse[2] = highByte(encRightVal);
    i2cResponse[3] = lowByte(encRightVal);
    i2cResponseLen += 4;

    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
  else if (requestedCmd == I2C_CMD_SET_SPEEDS) {
    i2cResponseLen = 0;

    int16_t left_motor = 0, right_motor = 0;
    left_motor = (int16_t((i2cArgs[1] << 8) | i2cArgs[0]));
    right_motor = (int16_t((i2cArgs[3] << 8) | i2cArgs[2]));
    motors.setSpeeds(left_motor, right_motor);

    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
  else if (requestedCmd == I2C_CMD_SET_LEFT_SPEED) {
    i2cResponseLen = 0;

    int16_t left_motor = 0, right_motor = 0;
    left_motor = (int16_t((i2cArgs[1] << 8) | i2cArgs[0]));
    motors.setLeftSpeed(left_motor);

    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
  else if (requestedCmd == I2C_CMD_SET_RIGHT_SPEED) {
    i2cResponseLen = 0;

    int16_t left_motor = 0, right_motor = 0;
    right_motor = (int16_t((i2cArgs[1] << 8) | i2cArgs[0]));
    motors.setLeftSpeed(right_motor);

    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
  else if (requestedCmd != 0) {
    Serial.println("Error: requested function is unsupported (main)");
    ledYellow(1);
    requestedCmd = 0;   // set requestd cmd to 0 disabling processing in next loop
  }
}


// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write(i2cResponse, i2cResponseLen);
}


// function that executes when master sends data (begin-end transmission)
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int cmdRcvd = -1;
  int argIndex = -1;
  argsCnt = 0;

  if (Wire.available()) {
    cmdRcvd = Wire.read();                 // receive first byte - command assumed
    while (Wire.available()) {             // receive rest of tramsmission from master assuming arguments to the command
      if (argIndex < I2C_MSG_ARGS_MAX) {
        argIndex++;
        i2cArgs[argIndex] = Wire.read();
      }
      else {
        Serial.println("Error: too many arguments");
        ledYellow(1);
      }
      argsCnt = argIndex + 1;
    }
  }
  else {
    Serial.println("Error: empty request");
    ledYellow(1);
    return;
  }
  // Command validation
  int fcnt = -1;
  for (int i = 0; i < sizeof(supportedI2Ccmd); i++) {
    if (supportedI2Ccmd[i] == cmdRcvd) {
      fcnt = i;
    }
  }

  if (fcnt < 0) {
    Serial.println("Error: command not supported");
    ledYellow(1);
    return;
  }
  requestedCmd = cmdRcvd;
  // now main loop code should pick up a command to execute and prepare required
  // response when master waits before requesting response
}
