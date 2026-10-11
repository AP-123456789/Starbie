#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------- BEGINNER SETTINGS --------------------
const int I2C_SDA_PIN = 6;
const int I2C_SCL_PIN = 7;
const int DHT_PIN = 3;
const int BUTTON_ONE_PIN = 4;
const int BUTTON_TWO_PIN = 5;

const bool USE_DHT11 = true; // Set false if your build has no DHT11.

#define DHT_TYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display(
    SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET
);

bool dhtAvailable = false;
bool dhtReadingValid = false;
bool mpuAvailable = false;

float temperatureC = 0;
float humidityPercent = 0;
unsigned long lastDHTRead = 0;

// -------------------- PET REACTIONS --------------------
enum Reaction {
    IDLE_REACTION,
    NAP_REACTION,
    RUN_REACTION,
    JUMP_REACTION,
    HEART_REACTION,
    SHAKE_REACTION
};

struct MenuItem {
    const char* label;
    int joy;
    int energy;
    int fullness;
    Reaction reaction;
};

const MenuItem MENU_ITEMS[] = {
    {"NAP",   1,  18, -4, NAP_REACTION},
    {"PLAY", 12,  -9, -5, RUN_REACTION},
    {"FEED",  3,   2, 18, JUMP_REACTION},
    {"PET",   7,   0,  0, HEART_REACTION}
};

const int MENU_COUNT = 4;

int selectedMenuItem = 0;
bool menuOpen = false;
bool statsVisible = false;

Reaction currentReaction = IDLE_REACTION;

unsigned long reactionStarted = 0;
unsigned long lastWander = 0;

int petX = 48;
int petDirection = 1;

int playLaps = 0;
bool playHearts = false;

unsigned long lastShake = 0;

float joy = 50;
float energy = 70;
float fullness = 60;

// -------------------- PET SPRITE --------------------
// Original 32 x 32 pet sprite.
const unsigned char PET_SPRITE[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x3C, 0x3C, 0x00,
    0x00, 0x7E, 0x7E, 0x00,
    0x00, 0xFF, 0xFF, 0x00,
    0x01, 0xFF, 0xFF, 0x80,
    0x03, 0xFF, 0xFF, 0xC0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x07, 0xE7, 0xE7, 0xE0,
    0x07, 0xE7, 0xE7, 0xE0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x03, 0xFF, 0xFF, 0xC0,
    0x01, 0xFF, 0xFF, 0x80,
    0x00, 0xFF, 0xFF, 0x00,
    0x00, 0x7F, 0xFE, 0x00,
    0x00, 0x3F, 0xFC, 0x00,
    0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x3F, 0xFC, 0x00,
    0x00, 0x77, 0xEE, 0x00,
    0x00, 0xE3, 0xC7, 0x00,
    0x01, 0xC1, 0x83, 0x80,
    0x03, 0x80, 0x01, 0xC0,
    0x03, 0x00, 0x00, 0xC0,
    0x03, 0x00, 0x00, 0xC0,
    0x03, 0x80, 0x01, 0xC0,
    0x01, 0xC0, 0x03, 0x80,
    0x00, 0xE0, 0x07, 0x00,
    0x00, 0x70, 0x0E, 0x00,
    0x00, 0x3C, 0x3C, 0x00,
    0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// -------------------- DRAWING HELPERS --------------------
void drawSparkle(int x, int y) {
    display.drawPixel(x, y - 2, SSD1306_WHITE);
    display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
    display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
    display.drawPixel(x - 2, y, SSD1306_WHITE);
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x + 2, y, SSD1306_WHITE);
    display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
    display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
    display.drawPixel(x, y + 2, SSD1306_WHITE);
}

void drawHeart(int x, int y) {
    display.fillCircle(x - 3, y, 4, SSD1306_WHITE);
    display.fillCircle(x + 3, y, 4, SSD1306_WHITE);

    display.fillTriangle(
        x - 7, y + 1,
        x + 7, y + 1,
        x, y + 10,
        SSD1306_WHITE
    );
}

// -------------------- START A REACTION --------------------
void startReaction(Reaction reaction, unsigned long now) {
    currentReaction = reaction;
    reactionStarted = now;

    if (reaction == RUN_REACTION) {
        petX = 0;
        petDirection = 1;
        playLaps = 0;
        playHearts = true;
    }

    if (reaction == NAP_REACTION) {
        playHearts = false;
    }
}

// -------------------- DRAW THE PET --------------------
void drawPet(unsigned long now) {
    int y = 24;

    // FEED: jump animation.
    if (currentReaction == JUMP_REACTION &&
        now - reactionStarted < 700) {
        y = 13;
    }

    // Shake reaction: move the pet up and down.
    if (currentReaction == SHAKE_REACTION &&
        now - reactionStarted < 650) {
        y = (now / 70) % 2 ? 21 : 27;
    }

    display.drawBitmap(
        petX, y,
        PET_SPRITE,
        32, 32,
        SSD1306_WHITE
    );

    // NAP: drifting Z symbols.
    if (currentReaction == NAP_REACTION) {
        int phase = (now - reactionStarted) / 350;

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(
            petX + 29,
            y - (phase % 3) * 3
        );
        display.print("z");

        display.setCursor(
            petX + 38,
            y - 7 - (phase % 3) * 3
        );
        display.print("z");
    }

    // PET: floating hearts.
    if (currentReaction == HEART_REACTION &&
        now - reactionStarted < 2400) {
        int rise = ((now - reactionStarted) / 100) % 18;

        drawHeart(petX + 17, y - 4 - rise);

        if (rise > 7) {
            drawHeart(petX + 27, y + 4 - (rise - 7));
        }
    }

    // PLAY: hearts appear while running.
    if (currentReaction == RUN_REACTION && playHearts) {
        int rise = ((now - reactionStarted) / 90) % 15;
        drawHeart(petX + 16, y - 3 - rise);
    }

    // SHAKE: sparkle reaction.
    if (currentReaction == SHAKE_REACTION &&
        now - reactionStarted < 650) {
        drawSparkle(petX + 3, y - 3);
        drawSparkle(petX + 28, y - 2);
    }

    // Sparkle trail.
    if (currentReaction != NAP_REACTION &&
        (now / 240) % 2 == 0) {
        drawSparkle(petX + 4, y - 4);
    }
}

// -------------------- RADIAL MENU --------------------
// NAP = top, PLAY = right, FEED = bottom, PET = left.
void drawMenu() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(35, 0);
    display.print("CHOOSE");

    const int cx[4] = {64, 91, 64, 11};
    const int cy[4] = {12, 30, 47, 30};

    for (int i = 0; i < MENU_COUNT; ++i) {
        int x = cx[i];
        int y = cy[i];

        if (i == selectedMenuItem) {
            display.fillCircle(
                x + 13, y + 2, 3, SSD1306_WHITE
            );
        } else {
            display.drawCircle(
                x + 13, y + 2, 3, SSD1306_WHITE
            );
        }

        display.setCursor(x, y + 7);
        display.print(MENU_ITEMS[i].label);
    }

    display.drawLine(64, 20, 64, 27, SSD1306_WHITE);
    display.drawLine(64, 36, 64, 42, SSD1306_WHITE);
    display.drawLine(35, 32, 53, 32, SSD1306_WHITE);
    display.drawLine(75, 32, 88, 32, SSD1306_WHITE);
}

// -------------------- STATS SCREEN --------------------
void drawStats() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("STARBIE STATS");

    display.setCursor(0, 14);
    display.print("Joy: ");
    display.println((int)joy);

    display.setCursor(0, 25);
    display.print("Energy: ");
    display.println((int)energy);

    display.setCursor(0, 36);
    display.print("Fullness: ");
    display.println((int)fullness);

    display.setCursor(0, 50);

    if (USE_DHT11 && dhtReadingValid) {
        display.print(temperatureC, 1);
        display.print("C ");
        display.print(humidityPercent, 0);
        display.print("% RH");
    } else {
        display.print("Climate unavailable");
    }
}

// -------------------- MPU6050 --------------------
void readMotionSensor(unsigned long now) {
    if (!mpuAvailable) {
        return;
    }

    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    float magnitude = sqrt(
        a.acceleration.x * a.acceleration.x +
        a.acceleration.y * a.acceleration.y +
        a.acceleration.z * a.acceleration.z
    );

    float delta = fabs(magnitude - 9.81f);

    float gyroMagnitude = sqrt(
        g.gyro.x * g.gyro.x +
        g.gyro.y * g.gyro.y +
        g.gyro.z * g.gyro.z
    );

    bool shaking = (delta > 4.0f || gyroMagnitude > 3.0f);

    // Shake wakes Starbie from NAP or triggers a shake reaction.
    if (shaking && now - lastShake > 900) {
        lastShake = now;

        if (currentReaction == NAP_REACTION) {
            startReaction(IDLE_REACTION, now);
        } else if (!menuOpen && !statsVisible) {
            startReaction(SHAKE_REACTION, now);
        }
    }

    // Tilt navigation while the menu is open.
    if (menuOpen) {
        float x = a.acceleration.x;
        float y = a.acceleration.y;

        if (fabs(x) > 3.0f || fabs(y) > 3.0f) {
            if (fabs(x) > fabs(y)) {
                selectedMenuItem = (x > 0) ? 1 : 3;
            } else {
                selectedMenuItem = (y > 0) ? 0 : 2;
            }
        }
    }
}

// -------------------- DHT11 --------------------
void readDHT11(unsigned long now) {
    if (!USE_DHT11 ||
        !dhtAvailable ||
        now - lastDHTRead < 2000) {
        return;
    }

    lastDHTRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
        humidityPercent = h;
        temperatureC = t;
        dhtReadingValid = true;
    } else {
        dhtReadingValid = false;
    }
}

// -------------------- BUTTON DEBOUNCING --------------------
bool buttonPressed(
    int pin,
    bool &lastState,
    unsigned long &lastChange
) {
    bool state = digitalRead(pin);
    unsigned long now = millis();

    if (state != lastState && now - lastChange > 35) {
        lastChange = now;
        lastState = state;

        if (state == LOW) {
            return true;
        }
    }

    return false;
}

unsigned long buttonOneChange = 0;
unsigned long buttonTwoChange = 0;

bool buttonOneLast = HIGH;
bool buttonTwoLast = HIGH;

// -------------------- SETUP --------------------
void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_ONE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_TWO_PIN, INPUT_PULLUP);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(
            "OLED initialization failed; check I2C wiring/address."
        );

        while (true) {
            delay(100);
        }
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Starting Starbie...");
    display.display();

    if (USE_DHT11) {
        dht.begin();
        dhtAvailable = true;
    }

    if (mpu.begin()) {
        mpuAvailable = true;

        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        Serial.println("MPU6050 initialized.");
    } else {
        Serial.println(
            "MPU6050 not found; tilt/shake controls require it."
        );
    }

    randomSeed(micros());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Starbie is ready!");
    display.display();

    delay(700);

    currentReaction = IDLE_REACTION;
    reactionStarted = millis();
    lastWander = millis();
}

// -------------------- MAIN LOOP --------------------
void loop() {
    unsigned long now = millis();

    readMotionSensor(now);
    readDHT11(now);

    // BUTTON 1:
    // First press opens menu.
    // Second press selects the highlighted item.
    if (buttonPressed(
            BUTTON_ONE_PIN,
            buttonOneLast,
            buttonOneChange)) {

        if (!menuOpen) {
            menuOpen = true;
            statsVisible = false;
            selectedMenuItem = 0;
        } else {
            joy = constrain(
                joy + MENU_ITEMS[selectedMenuItem].joy,
                0, 100
            );

            energy = constrain(
                energy + MENU_ITEMS[selectedMenuItem].energy,
                0, 100
            );

            fullness = constrain(
                fullness + MENU_ITEMS[selectedMenuItem].fullness,
                0, 100
            );

            startReaction(
                MENU_ITEMS[selectedMenuItem].reaction,
                now
            );

            menuOpen = false;
        }
    }

    // BUTTON 2: show or hide the stats screen.
    if (buttonPressed(
            BUTTON_TWO_PIN,
            buttonTwoLast,
            buttonTwoChange)) {

        if (!menuOpen) {
            statsVisible = !statsVisible;
        }
    }

    // IDLE: wander from side to side.
    if (!menuOpen &&
        !statsVisible &&
        currentReaction == IDLE_REACTION &&
        now - lastWander >= 90) {

        lastWander = now;

        petX += petDirection;

        if (petX <= 0) {
            petX = 0;
            petDirection = 1;
        }

        if (petX >= SCREEN_WIDTH - 32) {
            petX = SCREEN_WIDTH - 32;
            petDirection = -1;
        }
    }

    // PLAY: two full-width left-right-left laps.
    if (!menuOpen &&
        !statsVisible &&
        currentReaction == RUN_REACTION &&
        now - lastWander >= 22) {

        lastWander = now;
        petX += petDirection * 4;

        if (petX >= SCREEN_WIDTH - 32) {
            petX = SCREEN_WIDTH - 32;
            petDirection = -1;
        }

        if (petX <= 0 && petDirection < 0) {
            petX = 0;
            ++playLaps;

            if (playLaps >= 2) {
                playHearts = false;
                startReaction(IDLE_REACTION, now);
            } else {
                petDirection = 1;
            }
        }
    }

    // End temporary reactions.
    if (!menuOpen &&
        currentReaction == NAP_REACTION &&
        now - reactionStarted > 4500) {
        startReaction(IDLE_REACTION, now);
    }

    if (!menuOpen &&
        currentReaction == JUMP_REACTION &&
        now - reactionStarted > 900) {
        startReaction(IDLE_REACTION, now);
    }

    if (!menuOpen &&
        currentReaction == HEART_REACTION &&
        now - reactionStarted > 2500) {
        startReaction(IDLE_REACTION, now);
    }

    if (!menuOpen &&
        currentReaction == SHAKE_REACTION &&
        now - reactionStarted > 700) {
        startReaction(IDLE_REACTION, now);
    }

    // DRAW DISPLAY.
    display.clearDisplay();

    if (menuOpen) {
        drawMenu();
    } else if (statsVisible) {
        drawStats();
    } else {
        drawPet(now);
    }

    display.display();
    delay(20);
}