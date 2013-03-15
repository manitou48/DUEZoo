/*
  IRtest  DUE version
	 hack of Arduino version
	  http://www.arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
    use PWM to generate mark/space at 38Khz  pwm pin 6
	 pin 6 IR DUE  15ma max, IR led 940nm forward v: 1.2v current 100ma
	   3.3-1.2/0.015 = 140 ohm (use transistor for more power)
	  LED xmit   pin to 140ohm to +IR  - to grnd (short/flat)
	IR rcvr on pin 8  Sharp GP1UX311QS
	use timer 50us to count mark/space on input
	can run both xmit/recv at once 
*/

#include <stdint.h>
#include <stdio.h>
#include "IR_remote.h"

#define BOARD_LED_PIN 13
#define RecvPin 8

static uint8_t TCChanEnabled = 0;
static boolean pin_state = false ;
static Tc *chTC = RECV_TIMER;
static uint32_t chNo = RECV_CHNL;

static uint32_t PWMchan;
volatile unsigned long myticks;
int rawbuf[RAWBUF], rawlen;
uint8_t rcvstate;
int results_decode_type; // NEC, SONY, RC5, UNKNOWN
unsigned long results_value;
int results_bits; // Number of bits in decoded value


void TC3_Handler() {
	uint8_t irdata = (uint8_t)digitalRead(RecvPin);
	TC_GetStatus(RECV_TIMER, 0);    // reset interrupt
	myticks++;

  if (rawlen >= RAWBUF) {
    // Buffer overflow
    rcvstate = STATE_STOP;
  }
  switch(rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == MARK) {
      if (myticks < GAP_TICKS) {
        // Not big enough to be a gap.
        myticks = 0;
      }
      else {
        // gap just ended, record duration and start recording transmission
        rawlen = 0;
        rawbuf[rawlen++] = myticks;
        myticks = 0;
        rcvstate = STATE_MARK;
      }
     }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == SPACE) {   // MARK ended, record time
      rawbuf[rawlen++] = myticks;
      myticks = 0;
      rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == MARK) { // SPACE just ended, record it
      rawbuf[rawlen++] = myticks;
      myticks = 0;
      rcvstate = STATE_MARK;
    }
    else { // SPACE
      if (myticks > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        rcvstate = STATE_STOP;
      }
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == MARK) { // reset gap timer
      myticks = 0;
    }
    break;
  }

}

// set up recv timer  interrupt every 50us   10khz
void enableIRIn(){
        const uint32_t rc = VARIANT_MCK / 256 / 10000;

        if (!TCChanEnabled) {
            pmc_set_writeprotect(false);
            pmc_enable_periph_clk((uint32_t)RECV_IRQ);
            TC_Configure(chTC, chNo,
                TC_CMR_TCCLKS_TIMER_CLOCK4 |
                TC_CMR_WAVE |         // Waveform mode
                TC_CMR_WAVSEL_UP_RC ); // Counter running up and reset when equals to RC

            chTC->TC_CHANNEL[chNo].TC_IER=TC_IER_CPCS;  // RC compare interrupt
            chTC->TC_CHANNEL[chNo].TC_IDR=~TC_IER_CPCS;
            NVIC_EnableIRQ(RECV_IRQ);
            TCChanEnabled = 1;
        }
        TC_Stop(chTC, chNo);
        TC_SetRC(chTC, chNo, rc);    // set frequency
        TC_Start(chTC, chNo);


	rcvstate = STATE_IDLE;
	rawlen = 0;

	pinMode(RecvPin, INPUT);
}

void irrecv_resume() {
	rcvstate = STATE_IDLE;
	rawlen = 0;
}

long decodeSony() {
  long data = 0;
  if (rawlen < 2 * SONY_BITS + 2) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(rawbuf[offset], SONY_HDR_MARK)) {
    return ERR;
  }
  offset++;

  while (offset + 1 < rawlen) {
    if (!MATCH_SPACE(rawbuf[offset], SONY_HDR_SPACE)) {
      break;
    }
    offset++;
    if (MATCH_MARK(rawbuf[offset], SONY_ONE_MARK)) {
      data = (data << 1) | 1;
    }
    else if (MATCH_MARK(rawbuf[offset], SONY_ZERO_MARK)) {
      data <<= 1;
    }
    else {
      return ERR;
    }
    offset++;
  }

  // Success
  results_bits = (offset - 1) / 2;
  if (results_bits < 12) {
    results_bits = 0;
    return ERR;
  }
  results_value = data;
  results_decode_type = SONY;
  return DECODED;
}

void enableIROut(int khz) {
	int freq = 1000 * khz;
	int pwm_duty = 128;

	PWMchan = g_APinDescription[PWM_PIN].ulPWMChannel;
	pmc_enable_periph_clk( PWM_INTERFACE_ID );
	PWMC_ConfigureClocks( 255*freq, 0, VARIANT_MCK);
	PIO_Configure( g_APinDescription[PWM_PIN].pPort,
	           g_APinDescription[PWM_PIN].ulPinType,
               g_APinDescription[PWM_PIN].ulPin,
	           g_APinDescription[PWM_PIN].ulPinConfiguration);
	PWMC_ConfigureChannel(PWM_INTERFACE, PWMchan, PWM_CMR_CPRE_CLKA, 0, 0);
	PWMC_SetPeriod(PWM_INTERFACE, PWMchan, 255);
	PWMC_SetDutyCycle(PWM_INTERFACE, PWMchan, pwm_duty);
	PWMC_DisableChannel(PWM_INTERFACE, PWMchan);
}

void mark(int time) {
	PWMC_EnableChannel(PWM_INTERFACE, PWMchan);
	delayMicroseconds(time);
}

void space(int time) {
	 // ? make sure pin is low ?
	PWMC_DisableChannel(PWM_INTERFACE, PWMchan);
	delayMicroseconds(time);
}

void sendSony(unsigned long data, int nbits) {
  enableIROut(40);
  mark(SONY_HDR_MARK);
  space(SONY_HDR_SPACE);
  data = data << (32 - nbits);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(SONY_ONE_MARK);
      space(SONY_HDR_SPACE);
    }
    else {
      mark(SONY_ZERO_MARK);
      space(SONY_HDR_SPACE);
    }
    data <<= 1;
  }
}

void sendRaw(unsigned int buf[], int len, int khz)
{
  enableIROut(khz);
  for (int i = 0; i < len; i++) {
    if (i & 1) {
      space(buf[i]);
    }
    else {
      mark(buf[i]);
    }
  }
  space(0); // Just to be sure
}

void setup() {
	Serial.begin(9600);
	pinMode(RecvPin, INPUT);
	pinMode(PWM_PIN, OUTPUT);
	digitalWrite(PWM_PIN,LOW); // When not sending PWM, we want it low
	pinMode(BOARD_LED_PIN,OUTPUT);
	enableIRIn();
}

void loop() {
  long sonycmd[] = {0xA9A,0x91A,0x61A}; // power 0 7
  long cnt;

  Serial.println(" xmit");
  digitalWrite(BOARD_LED_PIN,HIGH);
  sendSony(sonycmd[0],SONY_BITS); 
  digitalWrite(BOARD_LED_PIN,LOW);
  delay(6);   // let gap time grow

  if (rcvstate == STATE_STOP) {
	if (decodeSony() ) {
		char str[128];
		sprintf(str,"sony decoded. value %0x  %d bits",results_value, results_bits);
		Serial.println(str);
	}
	Serial.print("rawlen "); Serial.println(rawlen,DEC);
	for (int i=0; i < rawlen; i++) {
        if (i%2) Serial.print(" ");
		Serial.println(rawbuf[i]*USECPERTICK, DEC);
        }
  	irrecv_resume();
  }
  delay(2000);
}
