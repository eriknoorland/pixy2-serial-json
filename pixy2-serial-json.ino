#include <ArduinoJson.h>
#include <Pixy2.h>

#define STATE_PAUSED 0
#define STATE_LINE 1
#define STATE_BLOCKS 2

int state = STATE_PAUSED;

Pixy2 pixy;

/**
 * Setup
 */
void setup() {
  Serial.begin(115200);
  pixy.init();

  pausedSetup();
}

/**
 * Loop
 */
void loop() {
  if (Serial.available() > 0) {
    if (Serial.peek() == 's') {
      Serial.read();
      state = Serial.parseInt();
      
      switch (state) {
        case STATE_PAUSED:
          pausedSetup();
          break;
        case STATE_LINE:
          lineSetup();
          break;
        case STATE_BLOCKS:
          blocksSetup();
          break;
      }
    }

    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  switch (state) {
    case STATE_PAUSED:
      // do nothing
      break;
    case STATE_LINE:
      lineLoop();
      break;
    case STATE_BLOCKS:
      blocksLoop();
      break;
  }
}

/**
 * Loop
 */
void pausedSetup() {
  pixy.setLamp(0, 0);
  pixy.setServos(500, 500);
  serializeStateChangeResponse(state);
}

/**
 * Line setup
 */
void lineSetup() {
  pixy.changeProg("line");
  pixy.setLamp(255, 255);
  pixy.setServos(500, 1000);
  serializeStateChangeResponse(state);
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
 * Blocks setup
 */
void blocksSetup() {
  pixy.changeProg("ccc");
  pixy.setLamp(255, 255);
  pixy.setServos(500, 600);
  serializeStateChangeResponse(state);
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

  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.createObject();
  root["code"] = 200;
  root["state"] = state;

  root.printTo(output);
  Serial.println(output);
}
