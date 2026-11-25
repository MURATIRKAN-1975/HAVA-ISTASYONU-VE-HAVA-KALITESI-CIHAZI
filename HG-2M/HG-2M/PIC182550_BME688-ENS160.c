#include <18F2550.h>
#device ADC=10

#fuses HS, NOWDT, NOPROTECT, NOLVP, NODEBUG, USBDIV, PLL5, CPUDIV1, VREGEN
#use delay(clock=20000000) // 20MHz Kristal (PLL ile 48MHz değil, doğrudan 20MHz çalışma)

// --- I2C Yapılandırması (Donanımsal I2C: SDA=B0, SCL=B1) ---
#use i2c(Master, Fast, sda=PIN_B0, scl=PIN_B1)

// --- 16x2 LCD Pin Tanımlamaları ---
#define LCD_ENABLE_PIN  PIN_B3
#define LCD_RS_PIN      PIN_B2
#define LCD_RW_PIN      PIN_C0 // Kullanılmıyorsa boşta bırakıp GND'ye bağlayın
#define LCD_DATA4       PIN_B4
#define LCD_DATA5       PIN_B5
#define LCD_DATA6       PIN_B6
#define LCD_DATA7       PIN_B7
#include <lcd.c>

// --- Sensör I2C Adresleri (8-Bit Format) ---
#define ENS160_ADDR_W   0xA6  // 0x53 << 1 (Yazma)
#define ENS160_ADDR_R   0xA7  // (0x53 << 1) | 1 (Okuma)

#define BME688_ADDR_W   0xEE  // 0x77 << 1 (Yazma)
#define BME688_ADDR_R   0xEF  // (0x77 << 1) | 1 (Okuma)

// --- ENS160 Register Adresleri ---
#define ENS160_REG_OPMODE      0x10
#define ENS160_REG_DATA_AQI    0x21
#define ENS160_REG_DATA_TVOC   0x22
#define ENS160_REG_DATA_ECO2   0x24
#define ENS160_OPMODE_STD      0x02 // Standart Ölçüm Modu

// --- BME688 Register Adresleri ---
#define BME688_REG_CTRL_HUM    0x72
#define BME688_REG_CTRL_MEAS   0x74
#define BME688_REG_TEMP_MSB    0x22
#define BME688_REG_PRESS_MSB   0x1F
#define BME688_REG_HUM_MSB     0x25

// Global Değişkenler
unsigned int8  aqi = 0;
unsigned int16 tvoc = 0;
unsigned int16 eco2 = 0;

unsigned int16 par_t1;
signed int16   par_t2;
signed int8    par_t3;
float          bme_temp = 0.0;
unsigned int16 bme_hum_raw = 0;

// --- Genel I2C Yazma Fonksiyonu ---
void i2c_write_reg(unsigned int8 dev_addr, unsigned int8 reg_addr, unsigned int8 data) {
   i2c_start();
   i2c_write(dev_addr);
   i2c_write(reg_addr);
   i2c_write(data);
   i2c_stop();
}

// --- Genel I2C Çoklu Byte Okuma Fonksiyonu ---
void i2c_read_bytes(unsigned int8 dev_addr_w, unsigned int8 reg_addr, unsigned int8 *buffer, unsigned int8 len) {
   unsigned int8 i;
   i2c_start();
   i2c_write(dev_addr_w);
   i2c_write(reg_addr);
   i2c_start(); // Repeated Start
   i2c_write(dev_addr_w | 0x01);
   for(i = 0; i < len; i++) {
      if(i == (len - 1))
         buffer[i] = i2c_read(0); // Son byte için NACK
      else
         buffer[i] = i2c_read(1); // ACK
   }
   i2c_stop();
}

// --- ENS160 Başlatma ve Okuma ---
void ens160_init() {
   delay_ms(50);
   i2c_write_reg(ENS160_ADDR_W, ENS160_REG_OPMODE, ENS160_OPMODE_STD);
   delay_ms(50);
}

void ens160_read_data() {
   unsigned int8 buf[5];
   // 0x21 (AQI), 0x22-0x23 (TVOC), 0x24-0x25 (eCO2) ardışık okunur
   i2c_read_bytes(ENS160_ADDR_W, ENS160_REG_DATA_AQI, buf, 5);
   
   aqi  = buf[0] & 0x07; // İlk 3 bit AQI (1-5 arası)
   tvoc = make16(buf[2], buf[1]); // Little Endian
   eco2 = make16(buf[4], buf[3]); // Little Endian
}

// --- BME688 Başlatma, Kalibrasyon ve Sıcaklık Hesaplama ---
void bme688_init() {
   unsigned int8 cal[3];
   // Nem oversampling x1
   i2c_write_reg(BME688_ADDR_W, BME688_REG_CTRL_HUM, 0x01);
   // Sıcaklık x2, Basınç x16, Forced Mode (0x01)
   i2c_write_reg(BME688_ADDR_W, BME688_REG_CTRL_MEAS, 0x55);
   
   // Sıcaklık kalibrasyon parametrelerini oku (0xE9-0xEA ve 0x8A-0x8B, 0x8C)
   i2c_read_bytes(BME688_ADDR_W, 0xE9, cal, 2);
   par_t1 = make16(cal[1], cal[0]);
   
   i2c_read_bytes(BME688_ADDR_W, 0x8A, cal, 3);
   par_t2 = make16(cal[1], cal[0]);
   par_t3 = (signed int8)cal[2];
}

void bme688_read_data() {
   unsigned int8 raw[8];
   unsigned int32 temp_adc;
   float var1, var2, t_fine;

   // Forced mode ölçümünü tetikle
   i2c_write_reg(BME688_ADDR_W, BME688_REG_CTRL_MEAS, 0x55);
   delay_ms(50); // Ölçümün tamamlanmasını bekle

   // 0x1F'den itibaren Basınç (3 byte), Sıcaklık (3 byte), Nem (2 byte) oku
   i2c_read_bytes(BME688_ADDR_W, BME688_REG_PRESS_MSB, raw, 8);

   // 20-bit Ham Sıcaklık Verisi
   temp_adc = ((unsigned int32)raw[3] << 12) | ((unsigned int32)raw[4] << 4) | ((unsigned int32)raw[5] >> 4);
   
   // 16-bit Ham Nem Verisi
   bme_hum_raw = make16(raw[6], raw[7]);

   // Bosch Datasheet Sıcaklık Kompanzasyon Formülü
   var1 = (((float)temp_adc / 16384.0) - ((float)par_t1 / 1024.0)) * (float)par_t2;
   var2 = ((((float)temp_adc / 131072.0) - ((float)par_t1 / 8192.0)) * (((float)temp_adc / 131072.0) - ((float)par_t1 / 8192.0))) * ((float)par_t3 * 16.0);
   t_fine = var1 + var2;
   bme_temp = t_fine / 5120.0;
}

void main() {
   setup_adc_ports(NO_ANALOGS); // Tüm pinler dijital I/O
   lcd_init();
   
   printf(lcd_putc, "\fSistem Basliyor");
   printf(lcd_putc, "\nBME688 & ENS160");
   
   ens160_init();
   bme688_init();
   delay_ms(1000);

   while(TRUE) {
      ens160_read_data();
      bme688_read_data();

      // 1. Satır: Sıcaklık ve Hava Kalite İndeksi (AQI: 1=Mükemmel, 5=Kötü)
      printf(lcd_putc, "\fT:%2.1fC  AQI:%u", bme_temp, aqi);
      
      // 2. Satır: eCO2 (ppm) ve TVOC (ppb)
      printf(lcd_putc, "\nCO2:%Lu TV:%Lu", eco2, tvoc);

      delay_ms(1000); // 1 saniyede bir güncelle
   }
}
