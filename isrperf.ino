// isrperf
//  interrupt latency using cycle counter 
//  fast port write  (bit band)
// output 13   input 2   B.25

volatile unsigned int t2;
unsigned int t1,t3;

void isr() {
	t2 = SysTick->VAL;   // or do this in PIOB_Handler WIntterupts.c 
} 

void setup() {
	pinMode(13,OUTPUT);
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;   // LED on
	Serial.begin(9600);
	t1 = SysTick->VAL;
	t3 = SysTick->VAL;
	Serial.println(t1-t3);
	t1 = SysTick->VAL;
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;
	t3 = SysTick->VAL;
	Serial.println(t1-t3);
	t1 = SysTick->VAL;
	digitalWrite(13,LOW);  // set bit
	t3 = SysTick->VAL;
	Serial.println(t1-t3);
	attachInterrupt(2,isr,RISING);
}

void loop(){
	char str[64];

	t2=0;
	t1 = SysTick->VAL;
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;   // LED on
	while(t2 == 0);  // wait
	t3 = SysTick->VAL;
	sprintf(str,"%d %d %d  %d %d  %d",t1,t2,t3,t1-t2,t2-t3,t1-t3);  
	Serial.println(str);
	digitalWrite(13,LOW);
	delay(3000);
}
