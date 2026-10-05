#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "config.h"

U8G2_SH1107_128X128_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

enum GrowthStage : uint8_t { STAGE_BABY, STAGE_CHILD, STAGE_TEEN, STAGE_ADULT, STAGE_ELDER };
enum Mood : uint8_t { MOOD_DEAD, MOOD_SICK, MOOD_SAD, MOOD_NEUTRAL, MOOD_HAPPY, MOOD_EXCITED, MOOD_SLEEPY };

struct PetState {
  int hunger;
  int happiness;
  int energy;
  int hygiene;
  int health;
  uint32_t ageSeconds;
  bool sick;
  bool sleeping;
  bool alive;
};

PetState pet;

String serialLine;
String lastCommand;
uint32_t lastCommandMs = 0;
uint32_t lastRenderMs = 0;
uint32_t lastAnimMs = 0;
uint32_t lastTickMs = 0;
uint32_t lastDecayMs = 0;
uint32_t nextEventMs = 0;
uint32_t statusUntilMs = 0;
uint32_t neglectedSinceMs = 0;
uint32_t sleepStartMs = 0;
bool animFrame = false;
String statusText = "Born healthy";
String deathReason = "";

int clampStat(int v) {
  if (v < STAT_MIN) return STAT_MIN;
  if (v > STAT_MAX) return STAT_MAX;
  return v;
}

GrowthStage getStage() {
  if (pet.ageSeconds < 90) return STAGE_BABY;
  if (pet.ageSeconds < 210) return STAGE_CHILD;
  if (pet.ageSeconds < 420) return STAGE_TEEN;
  if (pet.ageSeconds < 720) return STAGE_ADULT;
  return STAGE_ELDER;
}

const char* stageName(GrowthStage s) {
  switch (s) {
    case STAGE_BABY: return "Baby";
    case STAGE_CHILD: return "Child";
    case STAGE_TEEN: return "Teen";
    case STAGE_ADULT: return "Adult";
    default: return "Elder";
  }
}

Mood getMood() {
  if (!pet.alive) return MOOD_DEAD;
  if (pet.sleeping) return MOOD_SLEEPY;
  if (pet.sick) return MOOD_SICK;

  int avg = (pet.hunger + pet.happiness + pet.energy + pet.hygiene + pet.health) / 5;
  if (avg < 28) return MOOD_SAD;
  if (avg < 50) return MOOD_NEUTRAL;
  if (avg < 78) return MOOD_HAPPY;
  return MOOD_EXCITED;
}

const char* moodName(Mood m) {
  switch (m) {
    case MOOD_DEAD: return "Dead";
    case MOOD_SICK: return "Sick";
    case MOOD_SAD: return "Sad";
    case MOOD_NEUTRAL: return "Okay";
    case MOOD_HAPPY: return "Happy";
    case MOOD_EXCITED: return "Excited";
    case MOOD_SLEEPY: return "Sleepy";
    default: return "Unknown";
  }
}

void setStatus(const String& msg, uint32_t duration = STATUS_MSG_DURATION_MS) {
  statusText = msg;
  statusUntilMs = millis() + duration;
}

void printHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("  feed      - feed pet"));
  Serial.println(F("  play      - play with pet"));
  Serial.println(F("  clean     - clean pet"));
  Serial.println(F("  sleep     - toggle sleep / wake"));
  Serial.println(F("  medicine  - give medicine"));
  Serial.println(F("  stats     - print detailed stats"));
  Serial.println(F("  help      - show commands"));
  Serial.println(F("  reset     - restart game\n"));
}

void printStats() {
  Serial.println(F("---- PET STATS ----"));
  Serial.printf("Alive: %s\n", pet.alive ? "yes" : "no");
  Serial.printf("Stage: %s\n", stageName(getStage()));
  Serial.printf("Mood: %s\n", moodName(getMood()));
  Serial.printf("Age: %lu s\n", static_cast<unsigned long>(pet.ageSeconds));
  Serial.printf("Hunger: %d\n", pet.hunger);
  Serial.printf("Happiness: %d\n", pet.happiness);
  Serial.printf("Energy: %d\n", pet.energy);
  Serial.printf("Hygiene: %d\n", pet.hygiene);
  Serial.printf("Health: %d\n", pet.health);
  Serial.printf("Sick: %s\n", pet.sick ? "yes" : "no");
  Serial.printf("Sleeping: %s\n", pet.sleeping ? "yes" : "no");
  if (!pet.alive) Serial.printf("Death reason: %s\n", deathReason.c_str());
  Serial.println(F("-------------------"));
}

void startGame() {
  pet.hunger = 75;
  pet.happiness = 75;
  pet.energy = 75;
  pet.hygiene = 75;
  pet.health = 90;
  pet.ageSeconds = 0;
  pet.sick = false;
  pet.sleeping = false;
  pet.alive = true;

  serialLine = "";
  lastCommand = "";
  statusText = "A new pet is born!";
  statusUntilMs = millis() + STATUS_MSG_DURATION_MS;
  deathReason = "";
  neglectedSinceMs = 0;
  sleepStartMs = 0;
  animFrame = false;

  uint32_t now = millis();
  lastTickMs = now;
  lastDecayMs = now;
  lastRenderMs = 0;
  lastAnimMs = now;
  nextEventMs = now + random(18000, 35000);

  Serial.println(F("\n=== Tamagotchi Started ==="));
  printHelp();
  printStats();
}

void killPet(const String& reason) {
  pet.alive = false;
  pet.sleeping = false;
  deathReason = reason;
  setStatus("Game Over: " + reason, 15000);
  Serial.printf("GAME OVER: %s\n", reason.c_str());
  Serial.println(F("Type 'reset' to restart."));
}

void applyAction(const String& cmd) {
  if (cmd == "help") {
    printHelp();
    setStatus("Commands shown on Serial");
    return;
  }

  if (cmd == "stats") {
    printStats();
    setStatus("Stats printed");
    return;
  }

  if (cmd == "reset") {
    startGame();
    return;
  }

  if (!pet.alive) {
    setStatus("Pet is dead. Type reset.");
    return;
  }

  if (cmd == "feed") {
    pet.hunger = clampStat(pet.hunger + 20);
    pet.happiness = clampStat(pet.happiness + 5);
    pet.hygiene = clampStat(pet.hygiene - 3);
    setStatus("Yum! Fed.");
  } else if (cmd == "play") {
    if (pet.sleeping) {
      setStatus("Sleeping... use sleep to wake");
      return;
    }
    pet.happiness = clampStat(pet.happiness + 18);
    pet.energy = clampStat(pet.energy - 12);
    pet.hunger = clampStat(pet.hunger - 7);
    pet.hygiene = clampStat(pet.hygiene - 4);
    setStatus("Play time!");
  } else if (cmd == "clean") {
    pet.hygiene = clampStat(pet.hygiene + 28);
    pet.happiness = clampStat(pet.happiness - 2);
    setStatus("All clean.");
  } else if (cmd == "sleep") {
    pet.sleeping = !pet.sleeping;
    if (pet.sleeping) {
      sleepStartMs = millis();
      setStatus("Pet is sleeping...");
    } else {
      setStatus("Pet woke up.");
    }
  } else if (cmd == "medicine") {
    if (pet.sick) {
      pet.sick = false;
      pet.health = clampStat(pet.health + 16);
      setStatus("Medicine worked!");
    } else {
      pet.health = clampStat(pet.health - 5);
      setStatus("No need for medicine.");
    }
  } else {
    Serial.println(F("Unknown command. Type 'help'."));
    setStatus("Unknown cmd");
  }
}

void processCommand(const String& raw) {
  String cmd = raw;
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  uint32_t now = millis();
  if (now - lastCommandMs < COMMAND_DEBOUNCE_MS) return;
  if (cmd == lastCommand && now - lastCommandMs < COMMAND_REPEAT_GUARD_MS) return;

  lastCommand = cmd;
  lastCommandMs = now;
  applyAction(cmd);
}

void handleSerial() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      processCommand(serialLine);
      serialLine = "";
    } else {
      if (serialLine.length() < 64) serialLine += c;
    }
  }
}

void doRandomEvent() {
  if (!pet.alive) return;
  uint8_t r = random(0, 100);

  if (r < 20) {
    pet.hunger = clampStat(pet.hunger + 10);
    pet.happiness = clampStat(pet.happiness + 4);
    setStatus("Found a snack!");
  } else if (r < 40) {
    pet.hygiene = clampStat(pet.hygiene - 12);
    pet.happiness = clampStat(pet.happiness - 3);
    setStatus("Oops, got messy.");
  } else if (r < 58) {
    pet.health = clampStat(pet.health - 8);
    pet.happiness = clampStat(pet.happiness - 6);
    setStatus("Minor accident!");
  } else if (r < 78) {
    pet.happiness = clampStat(pet.happiness + 12);
    pet.energy = clampStat(pet.energy - 8);
    setStatus("Zoomies!");
  } else {
    pet.energy = clampStat(pet.energy + 10);
    setStatus("Quick rest.");
  }

  nextEventMs = millis() + random(18000, 35000);
}

void updateGame() {
  uint32_t now = millis();

  if (now - lastTickMs >= GAME_TICK_MS) {
    uint32_t steps = (now - lastTickMs) / GAME_TICK_MS;
    lastTickMs += steps * GAME_TICK_MS;
    if (pet.alive) pet.ageSeconds += steps;
  }

  if (pet.alive && pet.sleeping) {
    if (pet.energy >= 95 || (now - sleepStartMs) >= MAX_SLEEP_DURATION_MS) {
      pet.sleeping = false;
      setStatus("Naturally woke up.");
    }
  }

  if (pet.alive && now - lastDecayMs >= DECAY_INTERVAL_MS) {
    uint32_t steps = (now - lastDecayMs) / DECAY_INTERVAL_MS;
    lastDecayMs += steps * DECAY_INTERVAL_MS;

    for (uint32_t i = 0; i < steps; i++) {
      if (pet.sleeping) {
        pet.energy = clampStat(pet.energy + 5);
        pet.hunger = clampStat(pet.hunger - 1);
        pet.hygiene = clampStat(pet.hygiene - 1);
      } else {
        pet.energy = clampStat(pet.energy - 2);
        pet.hunger = clampStat(pet.hunger - 2);
        pet.hygiene = clampStat(pet.hygiene - 2);
        pet.happiness = clampStat(pet.happiness - 1);
      }

      if (pet.hunger < 25) pet.health = clampStat(pet.health - 2);
      if (pet.hygiene < 20) pet.health = clampStat(pet.health - 1);
      if (pet.energy < 15) pet.health = clampStat(pet.health - 1);
      if (pet.happiness < 20) pet.health = clampStat(pet.health - 1);
      if (pet.sick) {
        pet.health = clampStat(pet.health - 2);
        pet.happiness = clampStat(pet.happiness - 1);
      }

      if (!pet.sick && pet.hunger > 70 && pet.hygiene > 70 && pet.energy > 60 && pet.happiness > 60) {
        pet.health = clampStat(pet.health + 1);
      }
    }
  }

  if (pet.alive) {
    bool neglected = (pet.hunger < 15 || pet.hygiene < 15 || pet.energy < 10);
    if (neglected) {
      if (neglectedSinceMs == 0) neglectedSinceMs = now;
      if (!pet.sick && (now - neglectedSinceMs) >= NEGLECT_SICKNESS_TIME_MS) {
        pet.sick = true;
        setStatus("Pet became sick!");
      }
    } else {
      neglectedSinceMs = 0;
    }
  }

  if (pet.alive && now >= nextEventMs) doRandomEvent();

  if (pet.alive && pet.health <= 0) {
    killPet("Health reached zero");
  }

  if (statusUntilMs != 0 && now > statusUntilMs) {
    statusUntilMs = 0;
    statusText = pet.alive ? "Awaiting command..." : "Type reset to restart";
  }

  if (now - lastAnimMs >= ANIM_INTERVAL_MS) {
    animFrame = !animFrame;
    lastAnimMs = now;
  }
}

void drawBar(uint8_t x, uint8_t y, const char* label, int value) {
  const uint8_t w = 58;
  const uint8_t h = 8;
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(x, y + 6, label);
  u8g2.drawFrame(x + 16, y, w, h);
  uint8_t fill = static_cast<uint8_t>((clampStat(value) * (w - 2)) / 100);
  u8g2.drawBox(x + 17, y + 1, fill, h - 2);
}

void drawPetFace(Mood mood) {
  const int cx = 32;
  const int cy = 29;
  const int r = 21;

  u8g2.drawCircle(cx, cy, r);
  u8g2.drawCircle(cx, cy, r - 1);

  if (!pet.alive) {
    u8g2.drawLine(cx - 10, cy - 6, cx - 6, cy - 2);
    u8g2.drawLine(cx - 6, cy - 6, cx - 10, cy - 2);
    u8g2.drawLine(cx + 6, cy - 6, cx + 10, cy - 2);
    u8g2.drawLine(cx + 10, cy - 6, cx + 6, cy - 2);
    u8g2.drawLine(cx - 7, cy + 10, cx + 7, cy + 10);
    return;
  }

  if (pet.sleeping) {
    u8g2.drawStr(cx - 11, cy - 2, "- -");
    u8g2.drawStr(cx + 3, cy - 2, "- -");
    u8g2.drawStr(cx + 13, cy - 15, "z");
    if (animFrame) u8g2.drawStr(cx + 17, cy - 20, "z");
    u8g2.drawLine(cx - 7, cy + 10, cx + 7, cy + 10);
    return;
  }

  // Eyes
  if (animFrame && (mood == MOOD_HAPPY || mood == MOOD_EXCITED)) {
    u8g2.drawDisc(cx - 8, cy - 5, 2);
    u8g2.drawDisc(cx + 8, cy - 5, 2);
  } else {
    u8g2.drawBox(cx - 10, cy - 7, 4, 4);
    u8g2.drawBox(cx + 6, cy - 7, 4, 4);
  }

  // Mouth
  if (mood == MOOD_SAD || mood == MOOD_SICK) {
    u8g2.drawLine(cx - 6, cy + 10, cx + 6, cy + 10);
    u8g2.drawPixel(cx - 7, cy + 11);
    u8g2.drawPixel(cx + 7, cy + 11);
  } else if (mood == MOOD_HAPPY || mood == MOOD_EXCITED) {
    u8g2.drawLine(cx - 6, cy + 9, cx + 6, cy + 9);
    u8g2.drawPixel(cx - 5, cy + 10);
    u8g2.drawPixel(cx + 5, cy + 10);
    if (mood == MOOD_EXCITED) u8g2.drawPixel(cx, cy + 11);
  } else {
    u8g2.drawLine(cx - 4, cy + 10, cx + 4, cy + 10);
  }
}

void renderUI() {
  uint32_t now = millis();
  if (now - lastRenderMs < RENDER_INTERVAL_MS) return;
  lastRenderMs = now;

  Mood mood = getMood();
  int agePct = (pet.ageSeconds >= AGE_BAR_MAX_SECONDS) ? 100 : (pet.ageSeconds * 100) / AGE_BAR_MAX_SECONDS;

  u8g2.firstPage();
  do {
    u8g2.drawFrame(0, 0, 128, 128);

    // Header
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(4, 11, "ESP32 Tama");

    drawPetFace(mood);

    // Right info panel
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(64, 18, "Stage:");
    u8g2.drawStr(94, 18, stageName(getStage()));
    u8g2.drawStr(64, 28, "Mood:");
    u8g2.drawStr(94, 28, moodName(mood));
    u8g2.drawStr(64, 38, "Age(s):");
    char ageBuf[12];
    snprintf(ageBuf, sizeof(ageBuf), "%lu", static_cast<unsigned long>(pet.ageSeconds));
    u8g2.drawStr(99, 38, ageBuf);
    u8g2.drawStr(64, 48, "State:");
    u8g2.drawStr(94, 48, pet.alive ? (pet.sick ? "Sick" : "Alive") : "Dead");

    // Stat bars
    drawBar(3, 58, "Hn", pet.hunger);
    drawBar(3, 67, "Hp", pet.happiness);
    drawBar(3, 76, "En", pet.energy);
    drawBar(3, 85, "Hy", pet.hygiene);
    drawBar(3, 94, "Hl", pet.health);
    drawBar(3, 103, "Ag", agePct);

    // Status + hints
    u8g2.setFont(u8g2_font_4x6_tf);
    String show = statusText;
    if (show.length() > 31) show = show.substring(0, 31);
    u8g2.drawStr(2, 114, show.c_str());
    u8g2.drawStr(2, 122, "feed play clean sleep med");
    u8g2.drawStr(2, 127, "stats help reset");
  } while (u8g2.nextPage());
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  u8g2.setI2CAddress(OLED_ADDR_7BIT * 2); // U8g2 expects 8-bit address
  u8g2.begin();

  randomSeed(static_cast<uint32_t>(esp_random()));
  startGame();
}

void loop() {
  handleSerial();
  updateGame();
  renderUI();
}
