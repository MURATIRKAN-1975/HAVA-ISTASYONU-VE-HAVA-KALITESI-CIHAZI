//    CCS - C DERLEYÝCÝ PIC18F452 HG-2A HAVA ÝSTASYONU VE HAVA KALÝTESÝ CÝHAZI
//     MURAT IRKAN 2026/11  TMP117 HASSAS SICAKLIK SENSÖRÜ VE EKRAN ST7735 SITRONIX
#include <18F452.h>
#fuses HS,NOWDT,NOPROTECT,NOLVP// SÝGORTA AYARLARI
#use delay(crystal=20000000) // 20 MHz KRÝSTAL BAÐLANIYOR.!
// PIC18F452 TMP117 SENSOR SDA->C4 PÝN VE C3 SCL BAGLANIYOR.!
#use i2c(Master, Fast=400000, sda=PIN_C4, scl=PIN_C3, force_hw)
// ST7735  LCD EKRAN I2C BAGLANTI PÝNLERÝ
#define ST7735_CS   PIN_C0
#define ST7735_RST  PIN_C1
#define ST7735_DC   PIN_C2
#define ST7735_SDA  PIN_C5
#define ST7735_SCL  PIN_C3

// DONANIMSAL SPI BAGLANTISI ST7735
#use spi(MASTER, SPI1, MODE=0, BITS=8, STREAM=SPI_TFT)
// TMP117 HAFIZA ADRESLERÝ
#define TMP117_ADDR_WRITE 0x90
#define TMP117_ADDR_READ  0x91
#define TMP117_REG_TEMP   0x00

// FONKSÝYON TANIMLARI...
void st7735_write_cmd(int8 cmd);
void st7735_write_data(int8 data);
void st7735_init(void);
float tmp117_read_temperature(void);// TMP117 SICAKLIK  OKUYOR...

void main() {
   float tempC = 0.0;
   
   setup_adc_ports(NO_ANALOGS);
   st7735_init();
   
   while(TRUE) {
      tempC = tmp117_read_temperature();
      
      // Update data every 500ms
      delay_ms(500);
   }
}

// Read 16-bit Temperature from TMP117
float tmp117_read_temperature(void) {
   int8 msb = 0, lsb = 0;
   signed int16 raw_temp = 0;
   float celsius = 0.0;
   
   i2c_start();
   i2c_write(TMP117_ADDR_WRITE);
   i2c_write(TMP117_REG_TEMP);
   i2c_start();
   i2c_write(TMP117_ADDR_READ);
   msb = i2c_read(1); // ACK
   lsb = i2c_read(0); // NACK
   i2c_stop();
   
   raw_temp = make16(msb, lsb);
   // 1 LSB = 0.0078125 Â°C (or raw_temp / 128.0)
   celsius = (float)raw_temp * 0.0078125;
   
   return celsius;
}

// Low-level SPI helper for ST7735
void st7735_write_cmd(int8 cmd) {
   output_low(ST7735_DC);
   output_low(ST7735_CS);
   spi_xfer(SPI_TFT, cmd);
   output_high(ST7735_CS);
}

void st7735_write_data(int8 data) {
   output_high(ST7735_DC);
   output_low(ST7735_CS);
   spi_xfer(SPI_TFT, data);
   output_high(ST7735_CS);
}

void st7735_init(void) {
   output_high(ST7735_CS);
   output_low(ST7735_RST);
   delay_ms(50);
   output_high(ST7735_RST);
   delay_ms(50);
   
   st7735_write_cmd(0x01); // SWRESET (Software Reset)
   delay_ms(150);
   st7735_write_cmd(0x11); // SLPOUT (Exit Sleep)
   delay_ms(120);
   st7735_write_cmd(0x3A); // COLMOD (Color Format)
   st7735_write_data(0x05); // 16-bit/pixel (RGB565)
   st7735_write_cmd(0x29); // DISPON (Display On)
}
