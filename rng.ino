// DUE's TRNG  32 bits every 84 ticks
//  http://arduino.cc/forum/index.php/topic,129083.0.html
//#include <component/component_trng.h>
//#include "component_trng.h"
#define LED_PIN 13

Trng *trng;
// http://gurmeetsingh.wordpress.com/2008/08/05/fast-bit-counting-routines/
#define MASK_01010101 (((unsigned int)(-1))/3)
#define MASK_00110011 (((unsigned int)(-1))/5)
#define MASK_00001111 (((unsigned int)(-1))/17)
int bitcount (unsigned int n) {
    n = (n & MASK_01010101) + ((n >> 1) & MASK_01010101) ;
    n = (n & MASK_00110011) + ((n >> 2) & MASK_00110011) ;
    n = (n & MASK_00001111) + ((n >> 4) & MASK_00001111) ;
    return n % 255 ;
}


uint32_t trueRandom()
{
    static bool enabled = false;
    if (!enabled) {
        pmc_enable_periph_clk(ID_TRNG);
        TRNG->TRNG_IDR =  TRNG_IDR_DATRDY;   // disable interrupts
        TRNG->TRNG_CR = TRNG_CR_KEY(0x524e47) | TRNG_CR_ENABLE;
        enabled = true;
    }

    while (! (TRNG->TRNG_ISR & TRNG_ISR_DATRDY))
        ;
    return TRNG->TRNG_ODATA;
}
void setup() {
	unsigned int rng;
	pmc_enable_periph_clk(ID_TRNG);  // start clocking
	trng = (Trng *) 0x400BC000;
	trng->TRNG_IDR = TRNG_IDR_DATRDY; // disable interrupts
	trng->TRNG_CR = TRNG_CR_KEY(0x524e47) | TRNG_CR_ENABLE;
	pinMode(LED_PIN,OUTPUT);
	Serial.begin(9600);
}

void loop() {
	// logger();
	display();
}

void display() {
	unsigned int rng;
    unsigned int cnt=0, prev, current, ones, onescnt=0;
    static unsigned int bytes[256];
	char str[64];

	while (! (trng->TRNG_ISR & TRNG_ISR_DATRDY));  //wait til data ready
	prev = trng->TRNG_ODATA;
	while(1) {
		while (! (trng->TRNG_ISR & TRNG_ISR_DATRDY));  //wait til data ready
		current = trng->TRNG_ODATA;
      prev = prev ^ current;
      ones = bitcount(prev);
      onescnt += ones;
      bytes[current &0xff]++;
      bytes[(current>>8) &0xff]++;
      bytes[(current>>16) &0xff]++;
      bytes[(current>>24) &0xff]++;
      cnt++;
      if(cnt % 1000 == 0) {
        unsigned int i, min = 0xffffffff,mini,max=0,maxi;
        for (i=0; i< 256;i++) {
            if (bytes[i] < min) { mini = i; min = bytes[i];}
            if (bytes[i] > max) { maxi = i; max = bytes[i];}
        }
        sprintf(str,"%08x ones %d %f cnt %d   min %02x %d max %02x %d \n",
         current,ones,onescnt/(cnt*32.),cnt, mini,min,maxi,max);
		Serial.print(str);
	}
	prev=current;
  }
}

void logger() {
	// await start byte from host then start sending random numbers
	unsigned int rng;

	while(!Serial.available()){}   // wait for byte
	Serial.read();
	while(1) {
		while (! (trng->TRNG_ISR & TRNG_ISR_DATRDY));  //wait til data ready
		rng = trng->TRNG_ODATA;
		Serial.write((uint8_t *)&rng,4);
		digitalWrite(LED_PIN, rng & 1);  // random flash
	}
}
