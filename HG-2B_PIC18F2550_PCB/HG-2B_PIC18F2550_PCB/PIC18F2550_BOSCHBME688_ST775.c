//  PIC18F2550 HG-2B HAVA ÝSTASYONU  MURAT IRKAN PIC18F2550 GELÝÞTÝRME KARTI
#include <18F2550.h>
#fuses HSPLL, NOWDT, NOPROTECT, NODEBUG, USBDIV, PLL5, CPUDIV1, VREGEN
#use delay(clock=48000000)

// DONANIMSAL  SPI BAGLANTI AYARI  (Mode 0, Master)
#use spi(MASTER, SPI1, MODE=0, BITS=8, STREAM=SPI_BUS)

// ST7735 EKRAN PÝN AYARLARI..!
#define ST7735_CS   PIN_B2
#define ST7735_DC   PIN_B3
#define ST7735_RST  PIN_B4

// BME688 CÝP SEÇÝMÝ
#define BME688_CS   PIN_B5

void main() {
   // SPI BAGLANTISI ÝKÝSÝ ÝÇÝN EKRAN VE SENSÖR
   output_high(ST7735_CS);
   output_high(BME688_CS);

   // 1. ST7735 EKRAN HAZIRLIK YAP..
 
    //st7735_fillScreen(ST7735_BLACK); BURAYA  ST7735 KÜTÜPHANESÝ EKLE.!

   // 2.(BME688 0x61 KODU DOÐRULAMA KODU SENSÖRÜN)
   output_low(BME688_CS);
   spi_xfer(SPI_BUS, 0xD0 | 0x80); // AYAR MSB (0x80) for SPI OKUMA ÝÇÝN.!
   unsigned int8 chip_id = spi_xfer(SPI_BUS, 0x00);
   output_high(BME688_CS);

   while(TRUE) {
      // Trigger forced mode measurement on BME688
      // Read raw ADC registers (0x1F to 0x2D)
      // Apply integer compensation formulas
      // Draw values to ST7735
      delay_ms(1000);
   }
}
