#include <Arduino.h>

void showHistory() {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_CYAN);
    tft.println("LAST CALCULATIONS:");
    tft.setTextColor(ST77XX_WHITE);
    
    // Citim in SRAM fara a folosi ecranul TFT in acelasi timp (pt a preveni conflict SPI)
    String historyLines[8];
    int lineCount = 0;
    
    digitalWrite(TFT_CS, HIGH); // Dezactivam display-ul pe magistrala SPI
    
    File f = SD.open("calc.txt");
    if (f) {
        // Citim ultimele linii (daca fisierul e mare, ajungem la final)
        // Pt microcontroler, cea mai sigura varianta e sa citim tot si sa pastram ultimele 8
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim(); // stergem '\r' sau spatii
            if (line.length() > 0) {
                // Shift array left
                if (lineCount == 8) {
                    for(int i=0; i<7; i++) historyLines[i] = historyLines[i+1];
                    historyLines[7] = line;
                } else {
                    historyLines[lineCount] = line;
                    lineCount++;
                }
            }
        }
        f.close();
    }
    
    // Reactivam implicit folosirea tft (Adafruit gestioneaza CS intern cand dam .print, dar noi sigur l-am oprit inainte)
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
