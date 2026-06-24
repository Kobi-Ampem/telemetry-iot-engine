#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT22

// Hardware Pin Layout definitions
const int trigPin = 3;
const int echoPin = 4;
const int greenLed = 5;
const int yellowLed = 6;
const int redLed = 7;
const int buzzer = 8;
const int ldrPin = A0;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0;
const long interval = 500; // Run evaluation every 500ms

// Define human-readable names for our State Machine values
enum SystemState
{
    STATE_NORMAL,
    STATE_WARNING,
    STATE_CRITICAL
};

void setup()
{
    Serial.begin(115200);
    dht.begin();

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(yellowLed, OUTPUT);
    pinMode(redLed, OUTPUT);
    pinMode(buzzer, OUTPUT);

    lcd.init();
    lcd.backlight();
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        // SENSOR SAMPLING
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        long duration = pulseIn(echoPin, HIGH);
        float displacementMm = (duration * 0.343) / 2.0;

        int lightValue = analogRead(ldrPin);

        // Fail-safe sanity check
        if (isnan(temperature) || isnan(humidity))
        {
            lcd.setCursor(0, 0);
            lcd.print("SENSOR FAULT!  ");
            return;
        }

        // STATE CLASSIFICATION EVALUATION
        SystemState activeState = STATE_NORMAL;
        String stateLabel = "NORMAL";

        // Threshold Rules (Simulating extreme mining drill constraints)
        if (temperature > 60.0 || displacementMm > 150.0 || lightValue > 800)
        {
            activeState = STATE_CRITICAL;
            stateLabel = "CRITICAL";
        }
        else if (temperature > 40.0 || displacementMm > 80.0 || lightValue > 500)
        {
            activeState = STATE_WARNING;
            stateLabel = "WARNING";
        }

        // HARDWARE REACTION EXECUTION
        // Turn everything off first to cleanly toggle state changes
        digitalWrite(greenLed, LOW);
        digitalWrite(yellowLed, LOW);
        digitalWrite(redLed, LOW);
        noTone(buzzer);

        if (activeState == STATE_NORMAL)
        {
            digitalWrite(greenLed, HIGH);
            lcd.setCursor(0, 0);
            lcd.print("SYS: NORMAL     ");
            lcd.setCursor(0, 1);
            lcd.print("T:" + String((int)temperature) + "C D:" + String((int)displacementMm) + "mm  ");
        }
        else if (activeState == STATE_WARNING)
        {
            digitalWrite(yellowLed, HIGH);
            tone(buzzer, 440, 50); // Short acoustic chirp tone
            lcd.setCursor(0, 0);
            lcd.print("** WARNING **   ");
            lcd.setCursor(0, 1);
            lcd.print("CHECK TELEMETRY ");
        }
        else if (activeState == STATE_CRITICAL)
        {
            digitalWrite(redLed, HIGH);
            tone(buzzer, 880, 200); // Continuous piercing alarm tone
            lcd.setCursor(0, 0);
            lcd.print("!! CRITICAL !!  ");
            lcd.setCursor(0, 1);
            lcd.print("EMERGENCY SHUT  ");
        }

        // DATA PACKET BROADCAST
        // Stream complete data vectors out to data logger
        Serial.print("DATA,");
        Serial.print(temperature);
        Serial.print(",");
        Serial.print(humidity);
        Serial.print(",");
        Serial.print(displacementMm);
        Serial.print(",");
        Serial.print(lightValue);
        Serial.print(",");
        Serial.println(stateLabel);
    }
}
