// CCS C PIC18F452 MCU MURAT IRKAN 2026 HAVA İSTASYONU VE HAVA KALİTESİ CİHAZ KOD TEST..
#include <18F452.h>
#device adc=10

#fuses HS, NOWDT, NOPUT, NOPROTECT, NOBODEN, NOLVP
#use delay(clock=20000000) // 20 MHz crystal

// Configure Hardware I2C on RC4 (SDA) and RC3 (SCL)
#use i2c(Master, sonda=PIN_C4, scl=PIN_C3, fastapi=100000)
#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7) // UART for debugging

// I2C BME280 BAGLANTI ADRES TANIMI...
#define BME280_I2C_ADDR   0x76
#define ENS160_I2C_ADDR   0x52

// ENS160 ADRESLERİ
#define ENS160_REG_OPMODE     0x10
#define ENS160_REG_DATA_STATUS 0x20
#define ENS160_REG_DATA_AQI   0x21
#define ENS160_REG_DATA_TVOC  0x22
#define ENS160_REG_DATA_ECO2  0x24
#define ENS160_OPMODE_STD     0x02

// --- GENEL FONKSİYONLAR I2C ---
void Sensor_WriteReg(unsigned char addr, unsigned char reg, unsigned char val) {
    i2c_start();
    i2c_write(addr << 1);
    i2c_write(reg);
    i2c_write(val);
    i2c_stop();
}

void Sensor_ReadLen(unsigned char addr, unsigned char reg, unsigned char *buf, unsigned char length) {
    unsigned char i;
    i2c_start();
    i2c_write(addr << 1);
    i2c_write(reg);
    i2c_start();
    i2c_write((addr << 1) | 1);
    for(i = 0; i < length; i++) {
        buf[i] = i2c_read(i == (length - 1) ? 0 : 1);
    }
    i2c_stop();
}

void main() {
    unsigned char ens_status = 0;
    unsigned char aqi = 0;
    unsigned int16 tvoc = 0;
    unsigned int16 eco2 = 0;
    unsigned char rx_buf[4];
    
    delay_ms(200);
    printf("\r\nHAZIRLANIYOR BME280 ve ENS160 I2C baglantıyı paylaşıyor...\r\n");

    // 1. ens160 standart operasyon modu
    Sensor_WriteReg(ENS160_I2C_ADDR, ENS160_REG_OPMODE, 0x01); // bekle modu
    delay_ms(50);
    Sensor_WriteReg(ENS160_I2C_ADDR, ENS160_REG_OPMODE, ENS160_OPMODE_STD);
    delay_ms(100);

    // 2. opsiyonel: temel BME280 ayar kontrolu(BOSCH BME280 KİMLİK OKUMA ADRESİ KONTROLU 0xD0)
    unsigned char bme_id = 0;
    Sensor_ReadLen(BME280_I2C_ADDR, 0xD0, &bme_id, 1);
    printf("BME280 Chip ID (Expected 0x60 or 0x58): 0x%02X\r\n", bme_id);

    while(TRUE) {
        // ENS160 DURUMU KONTROL ET...
        Sensor_ReadLen(ENS160_I2C_ADDR, ENS160_REG_DATA_STATUS, &ens_status, 1);

        if(ens_status & 0x02) { // YENİ BİLGİLER...
            // HAVA KALİTE DEGERLERİ OKUNUYOR.....ENS160
            Sensor_ReadLen(ENS160_I2C_ADDR, ENS160_REG_DATA_AQI, &aqi, 1);
            
            Sensor_ReadLen(ENS160_I2C_ADDR, ENS160_REG_DATA_TVOC, rx_buf, 2);
            tvoc = make16(rx_buf[1], rx_buf[0]);

            Sensor_ReadLen(ENS160_I2C_ADDR, ENS160_REG_DATA_ECO2, rx_buf, 2);
            eco2 = make16(rx_buf[1], rx_buf[0]);

            // DEGERLER BİRLEŞTİ VE YAZILIYOR..
            printf("AQI: %u | TVOC: %u ppb | eCO2: %u ppm\r\n", aqi & 0x07, tvoc, eco2);
        } else {
            printf("ENS160 HAZIR DEGİL...\r\n");
        }

        delay_ms(2000);
    }
}