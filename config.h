#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 1. NETWORK CREDENTIALS (Wi-Fi)
// ==========================================
// Used for web dashboard, NTP time syncing, or local control panels
#define WIFI_SSID         "Hotspot Unavailable"
#define WIFI_PASSWORD     "qwertyuiop"

// ==========================================
// 2. HARDWARE PIN DECLARATIONS (ESP32)
// ==========================================
// IMPORTANT: Ensure these GPIO pins exactly match your new schematic diagram.

// I2C Screen Configuration (For Custom Faces/Expressions)
#define SCREEN_SDA_PIN    21  // ESP32 default I2C SDA
#define SCREEN_SCL_PIN    22  // ESP32 default I2C SCL

// Plant Monitoring Sensors
#define SOIL_MOISTURE_PIN 34  // Analog pin for measuring dirt wetness
#define LDR_LIGHT_PIN     35  // Analog pin for checking sunlight levels

// Interactive Inputs (Desktop Bot Controls)
#define CAP_TOUCH_PIN     4   // Touch pin for "petting" the bot
#define ACTION_BUTTON_PIN 15  // Physical button to cycle custom faces

// Audio Outputs
#define BUZZER_PIN        13  // For Tamagotchi sound effects and alerts

// ==========================================
// 3. DISPLAY & ANIMATION SETTINGS
// ==========================================
#define SCREEN_WIDTH      128 // Standard OLED width
#define SCREEN_HEIGHT     64  // Standard OLED height
#define EYE_BLINK_MS      250 // Duration of a single blink animation
#define BLINK_INTERVAL_MS 4000// Base time between natural blinks

// ==========================================
// 4. PLANT TAMAGOTCHI GAME MECHANICS
// ==========================================
// Threshold rules to determine the bot's mood faces
#define DRY_SOIL_THRESHOLD   1500  // Analog reading when plant needs water (Sad Face)
#define WET_SOIL_THRESHOLD   3000  // Analog reading when fully watered (Happy Face)
#define DARK_LIGHT_THRESHOLD 800   // Low light limit (Trigger Sleepy/Tired Face)

#define DEGRADE_INTERVAL_MS  60000 // How often (in ms) the tamagotchi's health ticks down

#endif // CONFIG_H
