// DUE DAC DMA clocked by TC0 chn 0  ISR 2 DMA buffers
// from https://github.com/TMRh20/AutoAnalogAudio
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define TMRHZ 44100

// 12-bit sine table 120 samples, 16-bit words for DAC
static uint16_t sine[] = {
  0x7ff, 0x86a, 0x8d5, 0x93f, 0x9a9, 0xa11, 0xa78, 0xadd, 0xb40, 0xba1,
  0xbff, 0xc5a, 0xcb2, 0xd08, 0xd59, 0xda7, 0xdf1, 0xe36, 0xe77, 0xeb4,
  0xeec, 0xf1f, 0xf4d, 0xf77, 0xf9a, 0xfb9, 0xfd2, 0xfe5, 0xff3, 0xffc,
  0xfff, 0xffc, 0xff3, 0xfe5, 0xfd2, 0xfb9, 0xf9a, 0xf77, 0xf4d, 0xf1f,
  0xeec, 0xeb4, 0xe77, 0xe36, 0xdf1, 0xda7, 0xd59, 0xd08, 0xcb2, 0xc5a,
  0xbff, 0xba1, 0xb40, 0xadd, 0xa78, 0xa11, 0x9a9, 0x93f, 0x8d5, 0x86a,
  0x7ff, 0x794, 0x729, 0x6bf, 0x655, 0x5ed, 0x586, 0x521, 0x4be, 0x45d,
  0x3ff, 0x3a4, 0x34c, 0x2f6, 0x2a5, 0x257, 0x20d, 0x1c8, 0x187, 0x14a,
  0x112, 0xdf, 0xb1, 0x87, 0x64, 0x45, 0x2c, 0x19, 0xb, 0x2,
  0x0, 0x2, 0xb, 0x19, 0x2c, 0x45, 0x64, 0x87, 0xb1, 0xdf,
  0x112, 0x14a, 0x187, 0x1c8, 0x20d, 0x257, 0x2a5, 0x2f6, 0x34c, 0x3a4,
  0x3ff, 0x45d, 0x4be, 0x521, 0x586, 0x5ed, 0x655, 0x6bf, 0x729, 0x794
};

uint32_t  dataReady, tcTicks, whichDac;

uint32_t frequencyToTimerCount(uint32_t frequency) {
  return VARIANT_MCK / 2UL / frequency;
}

void DACC_Handler(void) {
  // Called by the MCU when DAC is ready for data
  uint32_t status = dacc_get_interrupt_status(DACC);

  if ((status & DACC_ISR_ENDTX) == DACC_ISR_ENDTX) {
    if (dataReady < 1) {
      if (whichDac) {
        DACC->DACC_TPR = (uint32_t)sine;
        DACC->DACC_TCR = sizeof(sine) / sizeof(*sine);
        DACC->DACC_PTCR = DACC_PTCR_TXTEN;
      } else {
        DACC->DACC_TNPR = (uint32_t)sine;
        DACC->DACC_TNCR = sizeof(sine) / sizeof(*sine);
        DACC->DACC_PTCR = DACC_PTCR_TXTEN;
      }
      whichDac = !whichDac;
      dataReady = 1;
    } else {
      dacc_disable_interrupt(DACC, DACC_IER_ENDTX);
    }
  }
}

void timer_init(int frequency) {

  if (frequency > 0) {
    tcTicks = frequencyToTimerCount(frequency);
  }
  pmc_enable_periph_clk(TC_INTERFACE_ID);

  Tc * tc = TC0;
  TcChannel * t = &tc->TC_CHANNEL[0];
  t->TC_CCR = TC_CCR_CLKDIS;
  t->TC_IDR = 0xFFFFFFFF;
  t->TC_SR;
  t->TC_RC = tcTicks;
  t->TC_RA = tcTicks / 2;
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC;
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

void dac_xfer(uint8_t dacChannel, uint32_t samples) {
  while ((dacc_get_interrupt_status(DACC) & DACC_ISR_ENDTX) != DACC_ISR_ENDTX ) {}  // wait
  dataReady = 0;
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);
}

void setup() {
  Serial.begin(9600);
  delay(3000);

  pinMode(13, OUTPUT);

  // DAC init
  pmc_enable_periph_clk (DACC_INTERFACE_ID) ; // start clocking DAC
  dacc_reset(DACC);
  dacc_set_transfer_mode(DACC, 0);
  dacc_set_power_save(DACC, 0, 0);
  dacc_set_analog_control(DACC, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));
  dacc_set_trigger(DACC, 1);   // trigger TC0 chn 0

  //dacc_set_channel_selection(DACC, 0);
  dacc_enable_channel(DACC, 0);

  DACC->DACC_MR |= 1 << 8; // Refresh period of 24.3uS  (1024*REFRESH/DACC Clock)
  DACC->DACC_MR |= 1 << 21; // Max speed bit
  DACC->DACC_MR |= 1 << 20; // TAG mode   don't really use this, mono output

  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);

  //PRREG(DACC->DACC_MR);

  timer_init(TMRHZ);
  timer_init(0);   // why
#if 0
  Tc * tc = TC0;
  TcChannel * t = &tc->TC_CHANNEL[0];
  PRREG(t->TC_CV);
  PRREG(t->TC_CMR);
  PRREG(t->TC_RC);
  PRREG(t->TC_CV);
#endif
}

void loop() {
  digitalWrite(13, HIGH);
  dac_xfer(0, 120);
  digitalWrite(13, LOW);
}
