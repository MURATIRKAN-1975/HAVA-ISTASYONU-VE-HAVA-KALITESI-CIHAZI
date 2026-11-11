#include <18F452.h>
#device ICD=TRUE
#fuses HS,NOWDT,NOPUT,NOPROTECT,NODEBUG
#use delay(clock=20000000) // Assuming 20 MHz crystal

// Configure Software I2C (Bit-banged) on RC4 (SDA) and RC3 (SCL)
#use i2c(master, sda=PIN_C4, scl=PIN_C3, force_sw)

#define SCD40_I2C_ADDR 0x62  // 7-bit I2C address

// SCD40 Commands
#define CMD_START_PERIODIC_MEAS 0x21B1
#define CMD_READ_MEASUREMENT    0xEC05
#define CMD_STOP_MEASUREMENT    0x3093

// Function to calculate CRC8 for Sensirion sensors
unsigned int8 sensirion_crc8(unsigned int8 *data, unsigned int8 len) {
    unsigned int8 crc = 0xFF;
    unsigned int8 i, bit;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

// Send a 16-bit command to SCD40
void scd40_send_command(unsigned int16 cmd) {
    i2c_start();
    i2c_write(SCD40_I2C_ADDR << 1); // Write mode
    i2c_write(cmd >> 8);           // Command MSB
    i2c_write(cmd & 0xFF);         // Command LSB
    i2c_stop();
}

// Initialize sensor
void scd40_init() {
    delay_ms(1000); // Startup delay
    scd40_send_command(CMD_STOP_MEASUREMENT);
    delay_ms(500);
    scd40_send_command(CMD_START_PERIODIC_MEAS);
}

// Read data from SCD40
unsigned int8 scd40_read_measurement(unsigned int16 *co2, float *temperature, float *humidity) {
    unsigned int8 buffer[9];
    unsigned int16 raw_co2;
    unsigned int16 raw_temp;
    unsigned int16 raw_hum;

    scd40_send_command(CMD_READ_MEASUREMENT);
    delay_ms(1); // Processing delay

    i2c_start();
    i2c_read(SCD40_I2C_ADDR << 1 | 1); // Read mode
    
    // Read 3 words: CO2, Temp, Humidity (with CRC bytes)
    buffer[0] = i2c_read(0); 
    buffer[1] = i2c_read(0); 
    buffer[2] = i2c_read(0); 
    buffer[3] = i2c_read(0); 
    buffer[4] = i2c_read(0); 
    buffer[5] = i2c_read(0); 
    buffer[6] = i2c_read(0); 
    buffer[7] = i2c_read(0); 
    buffer[8] = i2c_read(1); // NACK last byte
    i2c_stop();

    // Verify CRCs
    if (sensirion_crc8(&buffer[0], 2) != buffer[2]) return 0; 
    if (sensirion_crc8(&buffer[3], 2) != buffer[5]) return 0; 
    if (sensirion_crc8(&buffer[6], 2) != buffer[8]) return 0; 

    raw_co2 = ((unsigned int16)buffer[0] << 8) | buffer[1];
    raw_temp = ((unsigned int16)buffer[3] << 8) | buffer[4];
    raw_hum = ((unsigned int16)buffer[6] << 8) | buffer[7];

    *co2 = raw_co2;
    *temperature = -45.0 + (175.0 * (float)raw_temp / 65536.0);
    *humidity = 100.0 * (float)raw_hum / 65536.0;

    return 1; // Success
}

void main() {
    unsigned int16 co2_val = 0;
    float temp_val = 0.0;
    float hum_val = 0.0;

    setup_uart(9600);
    printf("Initializing SCD40 (Software I2C)...\r\n");

    scd40_init();

    while(TRUE) {
        delay_ms(5000); // SCD40 periodic measurement update interval

        if (scd40_read_measurement(&co2_val, &temp_val, &hum_val)) {
            printf("CO2: %lu ppm\r\n", co2_val);
            printf("Temp: %3.2f C\r\n", temp_val);
            printf("Humidity: %3.2f %%\r\n\r\n", hum_val);
        } else {
            printf("CRC Error or Sensor not ready!\r\n");
        }
    }
}