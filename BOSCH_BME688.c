// MURAT IRKAN 2026 CCS C PIC18452 TEST DEMO
#include <18F452.h>
#device adc=10

#fuses HS, NOWDT, NOPUT, NOPROTECT, NOBODEN, NOLVP
#use delay(clock=20000000) // 20 MHz crystal

// BOSCH BME688  I2C  RC4 (SDA) VE  RC3 (SCL) BAGLANIYOR.
#use i2c(Master, sonda=PIN_C4, scl=PIN_C3, fastapi=100000)

#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7) // UART debugging

// BME688 I2C ADRESİ (0x76 if SDO w, 0x77 if SDO )
#define BME688_I2C_ADDR   0x76

// BME688 Register HARİTASI
#define BME688_REG_CHIP_ID    0xD0
#define BME688_REG_SOFT_RESET 0xE0

// Function to write a byte to a specific register
void BME688_WriteReg(unsigned int8 reg, unsigned int8 val) {
    i2c_start();
    i2c_write(BME688_I2C_ADDR << 1); // Write mode
    i2c_write(reg);
    i2c_write(val);
    i2c_stop();
}

// Function to read a single byte from a register
unsigned int8 BME688_ReadReg(unsigned int8 reg) {
    unsigned int8 val;
    i2c_start();
    i2c_write(BME688_I2C_ADDR << 1); // Write mode
    i2c_write(reg);
    i2c_start(); // Repeated start
    i2c_write((BME688_I2C_ADDR << 1) | 1); // Read mode
    val = i2c_read(0); // NACK last byte
    i2c_stop();
    return val;
}

void main() {
    unsigned int8 chip_id = 0;

    delay_ms(100); // GÜÇ AC STABILIZASYON 
    printf("\r\nPIC18F452 + BME688 HAZIRLIYOR...\r\n");

    // 1. RESETLE.!
    BME688_WriteReg(BME688_REG_SOFT_RESET, 0xB6);
    delay_ms(10);

    // 2. SENSOR KIMLIK OKUYOR (Should return 0x61 for BME688)
    chip_id = BME688_ReadReg(BME688_REG_CHIP_ID);
    
    printf("BME688 Chip ID Read: 0x%02X\r\n", chip_id);

    if(chip_id != 0x61) {// HATALI KIMLIK
        printf("ERROR: BME688 not detected or invalid ID! Check wiring & pull-ups.\r\n");
        while(TRUE) { delay_ms(1000); }
    } else {
        printf("SUCCESS: BME688 BULUNDU!\r\n");
    }

    while(TRUE) {
      // ANA OKUMA DÖNGÜSÜ
        delay_ms(2000);
    }
}