// MURAT IRKAN ENS160 HAVA KALİTESİ SENSÖR C/C++
#include <Wire.h>
#include <Adafruit_ENS160.h>

Adafruit_ENS160 ens160;

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  Serial.println("ENS160 SENSÖR HAVA KALİTE CİHAZ TEST");

  // ENS160 -> HAZIRLIK...
  if (!ens160.begin()) {
    Serial.println("ENS160 sensor bulunmadı, baglantıları kontrol et!");
    while (1) { delay(10); }
  }

  Serial.println("ENS160 başlıyor.");
  
  // STANDART OPERASYON MODU
  ens160.setOperatingMode(ENS160_OPMODE_STD);
}

void loop() {
  // HAZIRSA BILGILERI AL
  if (ens160.checkDataStatus()) {
    Serial.print("AQI (1-5): ");
    Serial.print(ens160.getAQI());
    Serial.print("\t | ");

    Serial.print("TVOC (ppb): ");
    Serial.print(ens160.getTVOC());
    Serial.print("\t | ");

    Serial.print("eCO2 (ppm): ");
    Serial.println(ens160.geteCO2());
  } else {
    Serial.println("HENÜZ HAZIR DEĞİL...");
  }

  delay(1000);
}