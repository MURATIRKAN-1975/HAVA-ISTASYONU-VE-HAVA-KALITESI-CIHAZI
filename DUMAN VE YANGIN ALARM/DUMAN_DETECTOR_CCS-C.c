//  HG-2A YANGIN DUMAN ALARMI MURAT IRKAN 
#include <18F452.h>
#fuses HS,NOWDT,NOPROTECT,NOLVP
#use delay(clock=20000000) // 20 MHz Kristal

// Pin Tanımlamaları
#define RE46_ALARM_PIN   PIN_B0
#define RE46_TEST_PIN    PIN_C0
#define STATUS_LED       PIN_D0

volatile int1 smoke_detected = 0;

// RB0 / INT0 Kesme Servis Rutini (Alarm Durumu)
#INT_EXT
void ext_isr(void)
{
   if(input(RE46_ALARM_PIN))
   {
      smoke_detected = 1;
   }
}

// Entegre Test Fonksiyonu
void RE46C190_SelfTest(void)
{
   output_high(RE46_TEST_PIN);
   delay_ms(100);
   output_low(RE46_TEST_PIN);
}

void main(void)
{
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   setup_spi(SPI_DISABLED);
   
   // Pin Yönlendirmeleri
   set_tris_b(0x01); // RB0 Giriş
   set_tris_c(0x00); // PORTC Çıkış
   set_tris_d(0x00); // PORTD Çıkış
   
   output_low(STATUS_LED);
   output_low(RE46_TEST_PIN);

   // Dış Kesme Ayarları (Yükselen Kenar Tetikleme)
   ext_int_edge(L_TO_H);
   clear_interrupt(INT_EXT);
   enable_interrupts(INT_EXT);
   enable_interrupts(GLOBAL);

   while(TRUE)
   {
      if(smoke_detected)
      {
         // Duman Algılandı: Flaşör / Büzzer Uyarısı
         output_high(STATUS_LED);
         delay_ms(200);
         output_low(STATUS_LED);
         delay_ms(200);
         
         // Alarm sinyali kesildiyse durumu sıfırla
         if(!input(RE46_ALARM_PIN))
         {
            smoke_detected = 0;
         }
      }
      else
      {
         output_low(STATUS_LED);
         delay_ms(500);
      }
   }
}
