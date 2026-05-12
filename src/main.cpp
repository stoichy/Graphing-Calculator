#include <Arduino.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// Pini Hardware
#define TFT_CS     10
#define TFT_RST    9
#define TFT_RS     8 
#define SD_CS      A4
#define BUZZER_PIN A2
#define POT_PIN    A3 

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_RS, TFT_RST);

// Tastatura
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '4', '7', 'C'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '='},
  {'+', '-', '*', '/'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, A0, A1};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Variabile Calculator
String inputString = "";
long firstNum = 0;
char op = ' ';
bool resultDisplayed = false;

void beep() { tone(BUZZER_PIN, 2800, 30); }

float readValue(String prompt) {
  String valStr = "";
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 20);
  tft.setTextColor(ST77XX_CYAN);
  tft.println(prompt);
  tft.setTextColor(ST77XX_WHITE);
  
  while (true) {
    char k = keypad.getKey();
    if (k) {
      beep();
      if (k >= '0' && k <= '9') {
        valStr += k;
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 20);
        tft.setTextColor(ST77XX_CYAN);
        tft.println(prompt);
        tft.setTextColor(ST77XX_WHITE);
        tft.println(valStr);
      } else if (k == '=') {
        return valStr.toFloat();
      } else if (k == 'C') {
        valStr = "";
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 20);
        tft.setTextColor(ST77XX_CYAN);
        tft.println(prompt);
      }
    }
  }
}

void plotFunction(bool isSine, float amplitude, float phase_input, int potVal) {
  tft.fillScreen(ST77XX_BLACK);
  
  // 1. ZOOM CARTEZIAN: Potentiometrul stabileste cat de mult din axa X vedem.
  // Mapam intre fereastra [-1.0, 1.0] (zoom maxim) si [-10.0, 10.0] (zoom out)
  float x_max = map(potVal, 0, 1023, 10, 100) / 10.0;
  float x_min = -x_max;
  
  // 2. Desenam axele X si Y
  tft.drawFastHLine(0, 80, 128, ST77XX_GREEN); 
  tft.drawFastVLine(64, 0, 160, ST77XX_GREEN); 
  
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  
  // Etichete Axa Y (Fixate de amplitudinea introdusa de tine)
  tft.setCursor(68, 15);  tft.print("+"); tft.print(amplitude, 0);
  tft.setCursor(68, 140); tft.print("-"); tft.print(amplitude, 0);
  tft.setCursor(68, 85);  tft.print("0");
  
  // Etichete Axa X (DINAMICE la zoom - afiseaza limitele ferestrei pe stanga/dreapta)
  tft.setCursor(0, 88); tft.print(x_min, 1);
  tft.setCursor(100, 88); tft.print(x_max, 1);
  
  // Text informativ
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(0,0);
  tft.print(isSine ? "SIN A:" : "COS A:");
  tft.print(amplitude, 0);
  tft.print(" F:");
  tft.print(phase_input, 0);
  
  float frecventa = 5.0; // Poti schimba in 1.0 cand vrei sa revii la calcul normal
  float phase_rad = phase_input; // In Desmos faza "12" se citeste direct in radiani

  float visual_amplitude = 60.0; 
  int last_y = -1; 
  
  for (int px = 0; px < 128; px++) {
    // Aflam valoarea matematica X reala (ex: -1.54) corespunzatoare acestui pixel
    float x_real = x_min + (px * (x_max - x_min) / 128.0);
    
    // Formula pura care iti genereaza graficul ( 5 * x + 12 )
    float unghi = (frecventa * x_real) + phase_rad;
    
    // Calculam functia (returneaza intre -1.0 si 1.0)
    float val = isSine ? sin(unghi) : cos(unghi);
    
    // Transformam valoarea in coordonata de pixeli pe ecran
    int y = 80 - (val * visual_amplitude);
    
    if(y >= 0 && y <= 160) {
      if (last_y != -1 && px > 0) {
        tft.drawLine(px - 1, last_y, px, y, ST77XX_WHITE);
        tft.drawLine(px - 1, last_y + 1, px, y + 1, ST77XX_WHITE); 
      } else {
        tft.drawPixel(px, y, ST77XX_WHITE);
      }
      last_y = y;
    } else {
      last_y = -1; 
    }
  }
}
void saveToHistory(String record) {
  File f = SD.open("calc.txt", FILE_WRITE);
  if (f) {
    f.println(record);
    f.close();
  }
}

void showHistory() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("LAST CALCULATIONS:");
  tft.setTextColor(ST77XX_WHITE);
  
  File f = SD.open("calc.txt");
  if (f) {
    int lines = 0;
    while (f.available() && lines < 8) {
      tft.println(f.readStringUntil('\n'));
      lines++;
    }
    f.close();
  } else {
    tft.println("No history.");
  }
  
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(0, 140);
  tft.print("Press C to exit");
  while(keypad.getKey() != 'C') { }
  beep();
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  
  if (!SD.begin(SD_CS)) {
    tft.setCursor(0,0); tft.print("SD Fail");
  }
  
  tft.setCursor(20, 70);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print("MATH BOX");
  delay(1500);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    beep();
    
    if (key >= '0' && key <= '9') {
      if (resultDisplayed) { 
        inputString = ""; op = ' '; firstNum = 0; resultDisplayed = false; 
      }
      inputString += key; 
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 40); tft.setTextSize(2);
      if (op != ' ') { 
        tft.print(firstNum); tft.print(String(op)); tft.print(inputString);
      } else {
        tft.print(inputString);
      }
      tft.setTextSize(1);
    }
    else if (key == '+' || key == '-' || key == '*' || key == '/') {
      if (inputString != "") firstNum = inputString.toInt();
      op = key;
      inputString = ""; 
      resultDisplayed = false;
      
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 40); tft.setTextSize(2);
      tft.print(firstNum); tft.print(String(op)); 
      tft.setTextSize(1);
    }
    else if (key == '=') {
      long secondNum = inputString.toInt();
      long result = 0;
      if (op == '+') result = firstNum + secondNum;
      if (op == '-') result = firstNum - secondNum;
      if (op == '*') result = firstNum * secondNum;
      if (op == '/') result = (secondNum != 0) ? firstNum / secondNum : 0;
      
      String finalCalc = String(firstNum) + op + String(secondNum) + "=" + String(result);
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 40); tft.setTextSize(2);
      tft.print(result);
      tft.setTextSize(1);
      
      saveToHistory(finalCalc);
      inputString = String(result);
      resultDisplayed = true;
    } 
    else if (key == 'C') {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 20);
      tft.println("1: History");
      tft.println("2: Sin Graph");
      tft.println("3: Cos Graph");
      
      char mode = ' ';
      while(mode == ' ') { mode = keypad.waitForKey(); }
      
      if (mode == '1') {
        showHistory();
      } 
      else if (mode == '2' || mode == '3') {
        float amp = readValue("Amplitudine (ex: 30)");
        float faza = readValue("Faza grade (ex: 90)");
        
        bool inGraphMode = true;
        int lastPot = -100; 
        
        while (inGraphMode) {
          int potVal = analogRead(POT_PIN);
          
          if (abs(potVal - lastPot) > 15) {
            lastPot = potVal;
            plotFunction(mode == '2', amp, faza, potVal);
          }
          
          char k = keypad.getKey();
          if (k == 'C') {
            beep();
            inGraphMode = false;
          }
        }
      }
      
      tft.fillScreen(ST77XX_BLACK);
      inputString = "";
      resultDisplayed = false;
    }
  }
}