// HG-2C(HAVA ÝSTASYONU ->PIC18F452 CCS-C DERLEYÝCÝ- SCD40-BOSCHBME280 VE ST7735 LCD EKRAN KODU.
//          MURAT IRKAN 2025/26   REV.C 
//******************************************************************************
#include <18F452.h>
#fuses HS,NOWDT,NOPROTECT,NOLVP
#use delay(clock=20000000) // 20 MHz KRÝSTAL

//  I2C BAGLANTISI ÝÇÝN SCD40 & BME280 SENSÖRLERÝN
#use i2c(Master, Fast=100000, sda=PIN_C4, scl=PIN_C3, force_hw)

// ST7735 LCD EKRAN PÝN BAÐLANTISI-> B PORTU B0-B1-B2
#define ST7735_CS   PIN_B0
#define ST7735_DC   PIN_B1
#define ST7735_RST  PIN_B2

// SÜRÜCÜLERÝ DAHÝL ET.
#include <st7735.c>   // ST7735 LCD KÜTÜPHANE
#include <bme280.c>   // BME280 SENSÖR

// SCD40 SENSÖR KOMUTLARI
#define SCD40_START_PERIODIC_MEAS  0x21B1// PERÝYODÝK ÖLÇME
#define SCD40_READ_MEASUREMENT     0xEC05// OKUMA ÖLÇÜM

void scd40_write_command(uint16_t cmd) {
   i2c_start();
   i2c_write(0xC4); // Address 0x62 + Write (0)
   i2c_write((uint8_t)(cmd >> 8));
   i2c_write((uint8_t)(cmd & 0xFF));
   i2c_stop();
}

int1 scd40_read_measurement(uint16_t *co2, float *temp, float *rh) {
   uint8_t data[9];
   uint8_t i;
   
   i2c_start();
   i2c_write(0xC4);
   i2c_write(0xEC);
   i2c_write(0x05);
   i2c_stop();
   
   delay_ms(1); // BEKLE...
   
   i2c_start();
   i2c_write(0xC5); // OKU MODU
   for(i = 0; i < 8; i++) {
      data[i] = i2c_read(1);
   }
   data[8] = i2c_read(0);
   i2c_stop();
   
   *co2 = ((uint16_t)data[0] << 8) | data[1];
   uint16_t raw_temp = ((uint16_t)data[3] << 8) | data[4];
   uint16_t raw_rh   = ((uint16_t)data[6] << 8) | data[7];
   
   *temp = -45.0 + 175.0 * ((float)raw_temp / 65536.0);
   *rh   = 100.0 * ((float)raw_rh / 65536.0);
   
   return 1;
}

void main() {
   uint16_t co2_ppm = 0;
   float scd_temp = 0.0, scd_rh = 0.0;
   float bme_temp = 0.0, bme_press = 0.0, bme_hum = 0.0;

   // EKRANI HAZIRLA
   ST7735_Init();
   ST7735_FillScreen(ST7735_BLACK);
   
   //  BME280 HAZIRLIK...
   bme280_init();
   
   // SCD40 SENSÖR NORMAL ÖLÇME
   scd40_write_command(SCD40_START_PERIODIC_MEAS);
   delay_ms(1000);

   while(TRUE) {
      // BME280 DEGERLERÝ OKUYOR..
      bme280_read_data(&bme_temp, &bme_press, &bme_hum);
      
      // SCD40 OKU ( 5 SANÝYEDE BÝR)
      scd40_read_measurement(&co2_ppm, &scd_temp, &scd_rh);
      
      // GÜNCELLE  ST7735 GÖRÜNTÜYÜ VER..
      ST7735_DrawString(5, 10,  "--- HG-2A HAVA DURUMU ---", ST7735_WHITE);
      // YAZMA FORMATI.
      
      delay_ms(5000); // SCD40 GÜNCELLEME DÖNGÜSÜ...
   }
}
