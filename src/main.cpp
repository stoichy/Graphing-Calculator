#include <Arduino.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>

// Pini Hardware
#define TFT_CS         10
#define TFT_RST        9
#define TFT_RS         8 
#define SD_CS          A4
#define BUZZER_PIN     A2
#define POT_PIN        A3 

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
bool resultDisplayed = false;

void beep() { tone(BUZZER_PIN, 2800, 30); }

int getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

float applyOperation(float a, float b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? (a / b) : 0;
    }
    return 0;
}

float evaluateExpression(String expr) {
    float values[16];
    char ops[16];
    int vIdx = 0;
    int oIdx = 0;
    int i = 0;
    while (i < expr.length()) {
        if (expr[i] == ' ') {
            i++; continue;
        }
        if (isdigit(expr[i])) {
            float val = 0;
            while (i < expr.length() && isdigit(expr[i])) {
                val = val * 10 + (expr[i] - '0');
                i++;
            }
            if (i < expr.length() && expr[i] == '.') {
                i++;
                float frac = 0.1;
                while (i < expr.length() && isdigit(expr[i])) {
                    val += (expr[i] - '0') * frac;
                    frac /= 10.0;
                    i++;
                }
            }
            values[vIdx++] = val;
        } else {
            while (oIdx > 0 && getPrecedence(ops[oIdx - 1]) >= getPrecedence(expr[i])) {
                if (vIdx < 2) break; 
                float b = values[--vIdx];
                float a = values[--vIdx];
                char op = ops[--oIdx];
                values[vIdx++] = applyOperation(a, b, op);
            }
            ops[oIdx++] = expr[i];
            i++;
        }
    }
    while (oIdx > 0) {
        if (vIdx < 2) break;
        float b = values[--vIdx];
        float a = values[--vIdx];
        char op = ops[--oIdx];
        values[vIdx++] = applyOperation(a, b, op);
    }
    return vIdx > 0 ? values[0] : 0;
}

void saveToHistory(String record) {
    // Ne asiguram ca ecranul nu interfereaza temporar
    digitalWrite(TFT_CS, HIGH); 
    
    File f = SD.open("calc.txt", FILE_WRITE);
    if (f) {
        f.println(record);
        f.close();
    } else {
        // Daca scrierea esueaza, vom emite un sunet lung ca avertisment
        tone(BUZZER_PIN, 500, 500); 
    }
}

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
    
    // 1. ZOOM CARTEZIAN (Stil Desmos/GeoGebra)
    // Mapam potentiometrul pentru a controla fereastra de vizualizare (limita axei X si Y)
    // Zoom maxim (fereastra mica): 1.0, Zoom out (fereastra mare): 20.0
    float x_max = map(potVal, 0, 1023, 10, 200) / 10.0;
    float x_min = -x_max;
    // Mentinem proportia corecta pe axa Y pentru un ecran 128x160 (160 / 128 = 1.25)
    float y_max = x_max * 1.25; 
    
    // 2. Desenam axele X si Y (Originea este mereu in centrul ecranului 64, 80)
    tft.drawFastHLine(0, 80, 128, ST77XX_GREEN); 
    tft.drawFastVLine(64, 0, 160, ST77XX_GREEN); 
    
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    
    // Calculam pasul optim pentru cadran (liniute) in functie de zoom
    float step = 5.0;
    if (x_max <= 2.0) step = 0.5;
    else if (x_max <= 5.0) step = 1.0;
    else if (x_max <= 10.0) step = 2.0;

    int decimals = (step < 1.0) ? 1 : 0; // afisam "0.5" cu 1 zecimala, restul fara ("1", "2")

    // Generam gradatiile si numerele pentru axa X
    for (float val = step; val <= x_max; val += step) {
        int px_pos = 64 + (val * 64.0 / x_max);
        int px_neg = 64 - (val * 64.0 / x_max);
        tft.drawFastVLine(px_pos, 78, 5, ST77XX_GREEN); // liniuta Jos-Sus
        tft.drawFastVLine(px_neg, 78, 5, ST77XX_GREEN);
        
        // Evitam sa printam foarte in margini sa nu taiem din text
        if (px_pos < 120) { tft.setCursor(px_pos - 4, 85); tft.print(val, decimals); }
        if (px_neg > 8)   { tft.setCursor(px_neg - 8, 85); tft.print(-val, decimals); }
    }

    // Generam gradatiile si numerele pentru axa Y
    for (float val = step; val <= y_max; val += step) {
        int py_pos = 80 - (val * 80.0 / y_max); // Matematic pozitiv (pe ecran se duce IN SUS)
        int py_neg = 80 + (val * 80.0 / y_max); // Matematic negativ (pe ecran se duce IN JOS)
        tft.drawFastHLine(62, py_pos, 5, ST77XX_GREEN); // liniuta Stanga-Dreapta
        tft.drawFastHLine(62, py_neg, 5, ST77XX_GREEN);
        
        if (py_pos > 10)  { tft.setCursor(68, py_pos - 3); tft.print(val, decimals); }
        if (py_neg < 150) { tft.setCursor(68, py_neg - 3); tft.print(-val, decimals); }
    }
    
    // Text informativ
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(0,0);
    tft.print(isSine ? "SIN A:" : "COS A:");
    tft.print(amplitude, 0);
    tft.print(" F:");
    tft.print(phase_input, 0);
    
    // Frecventa 1.0 pusa default pentru a arata corect matematic
    float frecventa = 1.0; 
    float phase_rad = phase_input; // Se pastreaza citirea in radiani
    
    int last_y = -1; 
    
    for (int px = 0; px < 128; px++) {
        // Aflam valoarea matematica X reala corespunzatoare acestui pixel
        float x_real = x_min + (px * (x_max - x_min) / 128.0);
        
        // Calculam functia trigonometrica cu faza si amplitudinea reale
        float unghi = (frecventa * x_real) + phase_rad;
        float y_real = amplitude * (isSine ? sin(unghi) : cos(unghi));
        
        // Transformam valoarea matematica in coordonata de pe ecran
        // (y_max corespunde la 80 de pixeli distanta de origine)
        int y = 80 - (y_real * (80.0 / y_max));
        
        // Afisam doar daca graficul nu "iese" din marginile ecranului
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

void showHistory() {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_CYAN);
    tft.println("LAST CALCULATIONS:");
    tft.setTextColor(ST77XX_WHITE);
    
    // 1. Citim de pe SD in memorie fara a apela ecranul (Garantam evitarea bus-conflictului SPI)
    String historyLines[8];
    int lineCount = 0;
    
    digitalWrite(TFT_CS, HIGH); // Taiem accesul la ecran pe SPI
    
    File f = SD.open("calc.txt");
    if (f) {
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim(); // Eliminam '\r' si spatii goale care arata ca linii in plus
            if (line.length() > 0) {
                // Mentinem doar ultimele 8 salvari facand "shift"
                if (lineCount == 8) {
                    for(int i = 0; i < 7; i++) historyLines[i] = historyLines[i+1];
                    historyLines[7] = line;
                } else {
                    historyLines[lineCount] = line;
                    lineCount++;
                }
            }
        }
        f.close();
    }
    
    // 2. Acum ca SD-ul si-a terminat treaba, dam drumul ecranului
    if (lineCount > 0) {
        for (int i = 0; i < lineCount; i++) {
            tft.println(historyLines[i]);
        }
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
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH); 
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);  
    
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST77XX_BLACK);
    
    tft.setCursor(0, 20);
    tft.setTextSize(1);
    
    // 2. Initializare SD cu raportare vizibila mai lunga
    if (!SD.begin(SD_CS)) {
        tft.setTextColor(ST77XX_RED);
        tft.println("SD Card FAIL / Not Found!");
        delay(3000); // Lasa mesajul 3 secunde ca sa il poti observa
    } else {
        tft.setTextColor(ST77XX_GREEN);
        tft.println("SD Card OK!");
        delay(1000);
    }
    
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(5, 70);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Felix C-256");
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
                inputString = ""; resultDisplayed = false; 
            }
            inputString += key; 
            
            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(0, 40); tft.setTextSize(2);
            tft.print(inputString);
            tft.setTextSize(1);
        }
        else if (key == '+' || key == '-' || key == '*' || key == '/') {
            if (resultDisplayed) {
                resultDisplayed = false;
            }
            inputString += key;
            
            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(0, 40); tft.setTextSize(2);
            tft.print(inputString);
            tft.setTextSize(1);
        }
        else if (key == '=') {
            float result = evaluateExpression(inputString);
            
            String finalCalc = inputString + "=" + String(result, 2);
            
            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(0, 40); tft.setTextSize(2);
            tft.print(result, 2);
            tft.setTextSize(1);
            
            saveToHistory(finalCalc);
            inputString = String(result, 2);
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
                float amp = readValue("Amplitude (any integer)");
                float faza = readValue("Phase in degrees (between 0 and 360)");
                
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