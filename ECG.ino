#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"

MAX30105 particleSensor;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
byte x;
byte y;
byte z;
byte lastx;
byte lasty;
long baseValue = 0;
long lastMin = 105000;
long lastMax= 103000;
long rollingMin = 105000;
long rollingMax= 103000;

const unsigned char Heart_Icon [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56, 
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

#define Buzzer 18
#define Button 25

void setup() {
  Serial.begin(115200);
  particleSensor.begin(Wire, I2C_SPEED_FAST);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  //Setup to sense a nice looking saw tooth on the plotter
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 50; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  //Take an average of IR readings at power up; this allows us to center the plot on start up
  const byte avgAmount = 30;
  long reading;
  for (byte x = 0 ; x < avgAmount ; x++){
    reading = particleSensor.getIR();

    // Find max IR reading in sample
    if (reading > lastMax){
      lastMax = reading;
    }

    // Find min IR reading in sample
    if (reading < lastMin){
      lastMin = reading;
    }
  }
  
  x = 0;
  y = 0;
  lastx = 0;
  lasty = 0;
  delay(2000);
  oled.clearDisplay();
  pinMode(Buzzer, OUTPUT);
  pinMode(Button, INPUT);
}

void loop() {

  long reading = particleSensor.getIR();

  if(reading > 100000){
    // Display is only 128 pixels wide, so if we're add the end of the display, clear the display and start back over
    if(x>127)  
    {
      oled.clearDisplay();
      oled.fillRect(0, 0, 128, 42, BLACK);
      x=0;
      lastx=0;
    }

    if (z > 30) {
      z = 0;
      lastMax = rollingMax;
      lastMin = rollingMin;
      rollingMin = 105000;
      rollingMax = 103000;
    }
  
    oled.setTextColor(WHITE);
    
    int y=40-(map(reading, lastMin, lastMax, 0, 40));   // Normalize the pleth waveform against the rolling IR min/max to keep waveform centered
    Serial.println(reading);
    oled.drawLine(lastx,lasty,x,y,WHITE);

    // Keep track of min/max IR readings to keep waveform centered
    if (reading > rollingMax){
      rollingMax = reading;
      digitalWrite(Buzzer, LOW);
    }

    if (reading < rollingMin){
      rollingMin = reading;
    }
    
    // Keep track of this IR reading so we can draw a line from it on the next reading
    lasty=y;
    lastx=x;

    oled.display();
    x++;
    z++;

  } else if(reading < 100000){
      oled.clearDisplay();
      oled.setTextSize(1);                    
      oled.setTextColor(WHITE);             
      oled.setCursor(30,5);                
      oled.println("Please Place "); 
      oled.setCursor(30,15);
      oled.println("your finger ");  
      oled.display();
      // Serial.print("Place your finger");
      digitalWrite(Buzzer, HIGH);
      oled.clearDisplay();
      x=0;
      lastx=x;
  }

  
}
