#include <PacketSerial.h>
#include <Pixy2.h>

#define REQUEST_START_FLAG 0xA6
#define REQUEST_COMMAND_STATE_IDLE 0x10
#define REQUEST_COMMAND_STATE_LINE 0x15
#define REQUEST_COMMAND_STATE_BLOCKS 0x20

#define RESPONSE_START_FLAG_1 0xA6
#define RESPONSE_START_FLAG_2 0x6A
#define RESPONSE_READY 0xFF
#define RESPONSE_LINE 0x15
#define RESPONSE_LINE_DATALENGTH 0x06
#define RESPONSE_BLOCKS 0x20
#define RESPONSE_BLOCKS_DATALENGTH 0x08
#define RESPONSE_STATE_CHANGE 0x25
#define RESPONSE_STATE_CHANGE_DATALENGTH 0x04

#define STATE_IDLE 0
#define STATE_LINE 1
#define STATE_BLOCKS 2

int state = STATE_IDLE;

uint8_t readyResponse[3] = {
  RESPONSE_START_FLAG_1,
  RESPONSE_START_FLAG_2,
  RESPONSE_READY
};

PacketSerial serial;
Pixy2 pixy;

/**
 * Set pan and tilt servos
 * @param {byte} pan
 * @param {byte} tilt
 */
void setServos(byte pan = 127, byte tilt = 0) {
  pixy.setServos(
    map(pan, 0, 255, 0, 1000),
    map(tilt, 0, 255, 500, 1000)
  );
}

/**
 * Set lamp brightness
 * @param {byte} brightness
 */
void setLamp(byte brightness = 0) {
  pixy.setLamp(brightness, brightness);
}

/**
 * Handles requests coming from the serial port
 * @param {uint8_t} buffer
 * @param {size_t} size
 */
void onPacketReceived(const uint8_t* buffer, size_t size) {
  byte startFlag = buffer[0];
  byte command = buffer[1];
  byte optionals;

  if (startFlag == REQUEST_START_FLAG) {
    switch (command) {
      case REQUEST_COMMAND_STATE_IDLE:
        setServos();
        setLamp();
        state = STATE_IDLE;
        sendStateChangeResponse(state);
        break;
        
      case REQUEST_COMMAND_STATE_LINE:
        setServos(buffer[2], buffer[3]);
        setLamp(buffer[4]);
        pixy.changeProg("line");
        state = STATE_LINE;
        sendStateChangeResponse(state);
        break;

      case REQUEST_COMMAND_STATE_BLOCKS:
        setServos(buffer[2], buffer[3]);
        setLamp(buffer[4]);
        pixy.changeProg("ccc");
        state = STATE_BLOCKS;
        sendStateChangeResponse(state);
        break;
    }
  }
}

/**
 * Line loop
 */
void lineLoop() {
  pixy.line.getMainFeatures();

  for (int8_t i = 0; i < pixy.line.numVectors; i++) {
    sendVectorResponse(pixy.line.vectors[i]);
  }
}

/**
 * Blocks loop
 */
void blocksLoop() {
  pixy.ccc.getBlocks();

  if (pixy.ccc.numBlocks) {
    for (int i = 0; i < pixy.ccc.numBlocks; i++) {
      Block block = pixy.ccc.blocks[i];

      uint8_t response[12] = {
        RESPONSE_START_FLAG_1,
        RESPONSE_START_FLAG_2,
        RESPONSE_BLOCKS,
        RESPONSE_BLOCKS_DATALENGTH,
        block.m_signature,
        block.m_x,
        block.m_y,
        block.m_width,
        block.m_height,
        block.m_index,
        block.m_angle,
        block.m_age
      };

      serial.send(response, sizeof(response));
    }
  }
}

/**
 * Send vector response
 * @param {Vector} vector
 */
void sendVectorResponse(Vector vector) {
  uint8_t response[10] = {
    RESPONSE_START_FLAG_1,
    RESPONSE_START_FLAG_2,
    RESPONSE_LINE,
    RESPONSE_LINE_DATALENGTH,
    vector.m_index,
    vector.m_flags,
    vector.m_x0,
    vector.m_y0,
    vector.m_x1,
    vector.m_y1
  };

  serial.send(response, sizeof(response));
}

/**
 * Send state change response
 * @param {int} state
 */
void sendStateChangeResponse(int state) {
  uint8_t response[8] = {
    RESPONSE_START_FLAG_1,
    RESPONSE_START_FLAG_2,
    RESPONSE_STATE_CHANGE,
    RESPONSE_STATE_CHANGE_DATALENGTH,
    0xC8,
    state,
    pixy.frameWidth,
    pixy.frameHeight
  };

  serial.send(response, sizeof(response));
}

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  serial.setStream(&Serial);
  serial.setPacketHandler(&onPacketReceived);

  pixy.init();

  // wait for serial port to connect. Needed for native USB
  while (!Serial) {}

  setServos();
  setLamp();
  
  serial.send(readyResponse, sizeof(readyResponse));

  sendStateChangeResponse(state);
}

/**
 * Loop
 */
void loop() {
  switch (state) {
    case STATE_LINE:
      lineLoop();
      break;
    case STATE_BLOCKS:
      blocksLoop();
      break;
  }

  serial.update();
}
