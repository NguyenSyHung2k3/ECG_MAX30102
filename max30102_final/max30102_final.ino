#include <Adafruit_GFX.h>    
#include <Adafruit_SSD1306.h> 
#include "MAX30105.h"           
#include "heartRate.h"          

MAX30105 particleSensor;

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;

double SpO2 = 0;
double ESpO2 = 90.0;
double FSpO2 = 0.7; 
double frate = 0.95; 
int i = 0;
int Num = 30;
#define FINGER_ON 7000    
#define MINIMUM_SPO2 90.0 
//OLED
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)

static const unsigned char PROGMEM heart_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,        
  0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
  0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
  0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,
};

//SPO2
static const unsigned char PROGMEM O2_bmp[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x3f, 0xc3, 0xf8, 0x00, 0xff, 0xf3, 0xfc,
  0x03, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0x7e,
  0x1f, 0x80, 0xff, 0xfc, 0x1f, 0x00, 0x7f, 0xb8, 0x3e, 0x3e, 0x3f, 0xb0, 0x3e, 0x3f, 0x3f, 0xc0,
  0x3e, 0x3f, 0x1f, 0xc0, 0x3e, 0x3f, 0x1f, 0xc0, 0x3e, 0x3f, 0x1f, 0xc0, 0x3e, 0x3e, 0x2f, 0xc0,
  0x3e, 0x3f, 0x0f, 0x80, 0x1f, 0x1c, 0x2f, 0x80, 0x1f, 0x80, 0xcf, 0x80, 0x1f, 0xe3, 0x9f, 0x00,
  0x0f, 0xff, 0x3f, 0x00, 0x07, 0xfe, 0xfe, 0x00, 0x0b, 0xfe, 0x0c, 0x00, 0x1d, 0xff, 0xf8, 0x00,
  0x1e, 0xff, 0xe0, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x1f, 0xe0, 0x00, 0x00,
  0x0f, 0xe0, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int Tonepin = 18; 

void setup() {

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.display();
  delay(3000);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) 
  {
    while (1);
  }

  byte ledBrightness = 0x7F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR
  int sampleRate = 800; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 215; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 
  particleSensor.enableDIETEMPRDY();

  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.setPulseAmplitudeGreen(0); 

  pinMode(Tonepin, OUTPUT);
}

void loop() {
  long irValue = particleSensor.getIR(); 
  if (irValue > FINGER_ON ) {
    
    if (checkForBeat(irValue) == true) {
      
      display.clearDisplay();
      display.drawBitmap(0, 0, heart_bmp, 32, 32, WHITE);
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(42, 10);
      display.drawBitmap(0, 35, O2_bmp, 32, 32, WHITE);
      display.setCursor(42, 40);
      if (beatAvg > 30) display.print(String(ESpO2) + "%");
      else display.print("---- %" );
      display.display();
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 35) {
        rates[rateSpot++] = (byte)beatsPerMinute; 
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
        
        digitalWrite(Tonepin, HIGH);

      }
      else{
        digitalWrite(Tonepin, LOW);
      }
    }
    uint32_t ir, red ;
    double fred, fir;
    particleSensor.check(); 
    if (particleSensor.available()) {
      i++;
      ir = particleSensor.getFIFOIR(); 
      red = particleSensor.getFIFORed(); 
      fir = (double)ir;
      fred = (double)red;
      aveir = aveir * frate + (double)ir * (1.0 - frate); 
      avered = avered * frate + (double)red * (1.0 - frate);
      sumirrms += (fir - aveir) * (fir - aveir);
      sumredrms += (fred - avered) * (fred - avered); 

      if ((i % Num) == 0) {
        double R = (sqrt(sumirrms) / aveir) / (sqrt(sumredrms) / avered);
        SpO2 = -23.3 * (R - 0.4) + 100;
        ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;
        if (ESpO2 <= MINIMUM_SPO2)
        {
          //digitalWrite(Tonepin, HIGH);
          //delay(1000); 
          ESpO2 = MINIMUM_SPO2;
        } 
        if (ESpO2 > 100) ESpO2 = 99.9;
        sumredrms = 0.0; sumirrms = 0.0; SpO2 = 0;
        i = 0;
      }
      particleSensor.nextSample(); 
    }

    display.clearDisplay();
    display.drawBitmap(5, 5, heart_bmp, 24, 21, WHITE);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(42, 10);
    display.print(beatAvg); display.println(" BPM");
    display.drawBitmap(0, 35, O2_bmp, 32, 32, WHITE);
    display.setCursor(42, 40);
    if (beatAvg > 30) display.print(String(ESpO2) + "%");
    else display.print("---- %" );
    display.display();
  }
  else {
    for (byte rx = 0 ; rx < RATE_SIZE ; rx++) rates[rx] = 0;
    beatAvg = 0; rateSpot = 0; lastBeat = 0;
    avered = 0; aveir = 0; sumirrms = 0; sumredrms = 0;
    SpO2 = 0; ESpO2 = 90.0;
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(30, 5);
    display.println("PLACE");
    display.setCursor(30, 35);
    display.println("FINGER");
    display.display();
    digitalWrite(Tonepin, HIGH);
  }
}
