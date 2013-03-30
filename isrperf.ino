// isrperf
//  interrupt latency using cycle counter 
//  fast port write  (bit band)  *(int *) 0x43c2066c =1;
// output 13   input 2   B.25

volatile unsigned int ti;

void isr() {
	ti = SysTick->VAL;   // or do this in PIOB_Handler WIntterupts.c 
} 

void setup() {
	unsigned int t1,t2,t3,t4,t5;
	char str[96];

	pinMode(13,OUTPUT);
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;   // LED on
	Serial.begin(9600);
	delay(123);			// quiesce
	t1 = SysTick->VAL;
	t2 = SysTick->VAL;
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;
	t3 = SysTick->VAL;
	digitalWrite(13,LOW);  // set bit
	t4 = SysTick->VAL;
	isr();
	t5 = SysTick->VAL;
	sprintf(str,"tick %d  set %d  write %d  fcn %d %d",
	  t1-t2,t2-t3,t3-t4, t4-ti,ti-t5);
	Serial.println(str);
	attachInterrupt(2,isr,RISING);
}

void loop(){
	unsigned int t1,t3;
	char str[64];

	ti=0;
	t1 = SysTick->VAL;
	g_APinDescription[13].pPort->PIO_SODR = g_APinDescription[13].ulPin;   // LED on
	while(ti == 0);  // wait
	t3 = SysTick->VAL;
	sprintf(str,"%d %d %d  isr %d %d",t1,ti,t3,t1-ti,ti-t3);  
	Serial.println(str);
	digitalWrite(13,LOW);
	delay(3000);
}
