#include <ArduinoJson.h>
#include <Pixy2.h>

#define REQUEST_START_FLAG 0xA6
#define MAX_REQUEST_DATA_LENGTH 5
#define REQUEST_COMMAND_STATE_IDLE 0x10
#define REQUEST_COMMAND_STATE_LINE 0x15
#define REQUEST_COMMAND_STATE_BLOCKS 0x20

#define STATE_IDLE 0
#define STATE_LINE 1
#define STATE_BLOCKS 2

int state = STATE_IDLE;

Pixy2 pixy;

/**
 * Set pan / tilt servos
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
 * Checks and handles to request coming from the serial port
 * @param {byte array} request
 */
void handleSerialRequest(byte request[]) {
  byte startFlag = request[0];
  byte command = request[1];
  byte optionals;

  if (startFlag == REQUEST_START_FLAG) {
    switch (command) {
      case REQUEST_COMMAND_STATE_IDLE:
        setServos();
        setLamp();
        state = STATE_IDLE;
        serializeStateChangeResponse(state);
        break;
        
      case REQUEST_COMMAND_STATE_LINE:
        setServos(request[2], request[3]);
        setLamp(request[4]);
        pixy.changeProg("line");
        state = STATE_LINE;
        serializeStateChangeResponse(state);
        break;

      case REQUEST_COMMAND_STATE_BLOCKS:
        setServos(request[2], request[3]);
        setLamp(request[4]);
        pixy.changeProg("ccc");
        state = STATE_BLOCKS;
        serializeStateChangeResponse(state);
        break;
    }
  }
}

/**
 * Check the serial line for requests
 */
void checkSerialRequest() {
  byte request[MAX_REQUEST_DATA_LENGTH] = {};

  if (Serial.available() > 0) {
    int i = 0;

    while (Serial.available() > 0) {
      request[i] = Serial.read();
      i++;
      delay(1);
    }

    handleSerialRequest(request);

    // reset request array
    for (int i = 0; i < MAX_REQUEST_DATA_LENGTH; i++) {
      request[i] = (char)0;
    }
  }
}

/**
 * 
 */
void sendReadyEvent() {
  String output;

  const size_t capacity = JSON_OBJECT_SIZE(1);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();
  root["status"] = "ready";

  root.printTo(output);
  Serial.println(output);
}

/**
 * Line loop
 */
void lineLoop() {
  pixy.line.getMainFeatures();

  for (int8_t i = 0; i < pixy.line.numVectors; i++) {
    serializeVector(pixy.line.vectors[i]);
  }
}

/**
 * Blocks loop
 */
void blocksLoop() {
  String output;
  pixy.ccc.getBlocks();

  if (pixy.ccc.numBlocks) {
    const size_t bufferSize = JSON_ARRAY_SIZE(pixy.ccc.numBlocks) + JSON_OBJECT_SIZE(pixy.ccc.numBlocks * 8);
    
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonArray& blocks = jsonBuffer.createArray();
    
    for (int i = 0; i < pixy.ccc.numBlocks; i++) {
      Block block = pixy.ccc.blocks[i];
      JsonObject& jsonBlock = blocks.createNestedObject();
      jsonBlock["signature"] = block.m_signature;
      jsonBlock["x"] = block.m_x;
      jsonBlock["y"] = block.m_y;
      jsonBlock["width"] = block.m_width;
      jsonBlock["height"] = block.m_height;
      jsonBlock["index"] = block.m_index;
      jsonBlock["angle"] = block.m_angle;
      jsonBlock["age"] = block.m_age;
    }

    blocks.printTo(output);
    Serial.println(output);
  }
}

/**
 * Serialize vector
 * @param {Vector} vector
 */
void serializeVector(Vector vector) {
  String output;

  const size_t bufferSize = JSON_OBJECT_SIZE(6);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["index"] = vector.m_index;
  root["flags"] = vector.m_flags;
  root["x0"] = vector.m_x0;
  root["y0"] = vector.m_y0;
  root["x1"] = vector.m_x1;
  root["y1"] = vector.m_y1;

  root.printTo(output);
  Serial.println(output);
}

/**
 * Serialize state change response
 * @param {int} state
 */
void serializeStateChangeResponse(int state) {
  String output;

  const size_t capacity = JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();
  root["code"] = 200;
  root["state"] = state;
  root["frameWidth"] = pixy.frameWidth;
  root["frameHeight"] = pixy.frameHeight;

  root.printTo(output);
  Serial.println(output);
}

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  pixy.init();

  // wait for serial port to connect. Needed for native USB
  while (!Serial) {}

  setServos();
  setLamp();
  sendReadyEvent();
  serializeStateChangeResponse(state);
}

/**
 * Loop
 */
void loop() {
  checkSerialRequest();

  switch (state) {
    case STATE_LINE:
      lineLoop();
      break;
    case STATE_BLOCKS:
      blocksLoop();
      break;
  }
}
