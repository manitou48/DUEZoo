

/**********************************************************************
 use DMA for memcpy memset
 DUE AHB DMA (ch 24) has 6 channels, 3/5 have 32B FIFO (others 8)
 ASAP_CFG faster than ALAP_CFG
 use IRQ to do interleave tests
 */


// mem2mem works on all channels
#define DMAC_MEMCH 5
#define DMAC_BTC DMAC_EBCIER_BTC5

volatile static unsigned long t3,DMADONE=0;

void DMAC_Handler() {
	uint32_t status = DMAC->DMAC_EBCISR;   // clear
	t3=micros();
	DMADONE = 1;
}

/** Disable DMA Controller. */
static void dmac_disable() {
  DMAC->DMAC_EN &= (~DMAC_EN_ENABLE);
}
/** Enable DMA Controller. */
static void dmac_enable() {
  DMAC->DMAC_EN = DMAC_EN_ENABLE;
}
/** Disable DMA Channel. */
static void dmac_channel_disable(uint32_t ul_num) {
  DMAC->DMAC_CHDR = DMAC_CHDR_DIS0 << ul_num;
}
/** Enable DMA Channel. */
static void dmac_channel_enable(uint32_t ul_num) {
  DMAC->DMAC_CHER = DMAC_CHER_ENA0 << ul_num;
}
/** Poll for transfer complete. */
static bool dmac_channel_transfer_done(uint32_t ul_num) {
  return (DMAC->DMAC_CHSR & (DMAC_CHSR_ENA0 << ul_num)) ? false : true;
}


void memcpy32(uint32_t *dst, uint32_t *src, uint32_t n) {
  DMADONE=0;
  dmac_channel_disable(DMAC_MEMCH);
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_SADDR = (uint32_t)src;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_DADDR = (uint32_t)dst;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_DSCR =  0;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CTRLA = n |
    DMAC_CTRLA_SRC_WIDTH_WORD | DMAC_CTRLA_DST_WIDTH_WORD;

  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CTRLB =  DMAC_CTRLB_SRC_DSCR |
    DMAC_CTRLB_DST_DSCR | DMAC_CTRLB_FC_MEM2MEM_DMA_FC |
    DMAC_CTRLB_SRC_INCR_INCREMENTING | DMAC_CTRLB_DST_INCR_INCREMENTING;

  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CFG = DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ASAP_CFG;

  dmac_channel_enable(DMAC_MEMCH);
}

void memset32(uint32_t *dst, uint32_t word, uint32_t n) {
  DMADONE=0;
  dmac_channel_disable(DMAC_MEMCH);
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_SADDR = (uint32_t)&word;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_DADDR = (uint32_t)dst;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_DSCR =  0;
  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CTRLA = n |
    DMAC_CTRLA_SRC_WIDTH_WORD | DMAC_CTRLA_DST_WIDTH_WORD;

  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CTRLB =  DMAC_CTRLB_SRC_DSCR |
    DMAC_CTRLB_DST_DSCR | DMAC_CTRLB_FC_MEM2MEM_DMA_FC |
    DMAC_CTRLB_SRC_INCR_FIXED | DMAC_CTRLB_DST_INCR_INCREMENTING;

  DMAC->DMAC_CH_NUM[DMAC_MEMCH].DMAC_CFG = DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ASAP_CFG;

  dmac_channel_enable(DMAC_MEMCH);
}

#define WORDS 1024
uint32_t src[WORDS],dst[WORDS],a[WORDS],b[WORDS];

void setup(){
	Serial.begin(9600);
	pmc_enable_periph_clk(ID_DMAC);
	dmac_disable();
	 DMAC->DMAC_GCFG = DMAC_GCFG_ARB_CFG_FIXED;
	 NVIC_DisableIRQ(DMAC_IRQn);
	 NVIC_SetPriority(DMAC_IRQn, 0);
	 NVIC_EnableIRQ(DMAC_IRQn);
	 uint32_t status = DMAC->DMAC_EBCISR;  // clear interrupts
	 DMAC->DMAC_EBCIER |= DMAC_BTC; // enable interrupt
	dmac_enable();
}

void loop(){
	int i,t1,t2,t4;
	uint32_t c[WORDS],d[WORDS]; // SRAM1  systick stack interference?
	
	for (i=0;i<WORDS;i++){
		a[i]=c[i]=dst[i]=0;
		b[i]=d[i]=src[i]=i;
	}
	memcpy32(dst,src,WORDS);
    while (!dmac_channel_transfer_done(DMAC_MEMCH)) {}
        t1=micros();
        memcpy32(dst,src,WORDS);
        while (!dmac_channel_transfer_done(DMAC_MEMCH)) {}
        t2 = micros() - t1;
        Serial.print("memcpy32 ");Serial.println(t2,DEC);
        t1=micros();
        memcpy(dst,src,4*WORDS);
        t2 = micros() - t1;
        Serial.print("memcpy ");Serial.println(t2,DEC);
        t1=micros();
		memcpy32(dst,src,WORDS);
		t2=micros();
		memcpy(c,d,4*WORDS);
		t4 = micros();
		while (!dmac_channel_transfer_done(DMAC_MEMCH)) {}
        Serial.println("t1 t2 t3 t4 ");
        Serial.println(t1,DEC);
        Serial.println(t2,DEC);Serial.println(t3,DEC);Serial.println(t4,DEC);
        t3 = t3 - t1;
        Serial.print("memcpy32+ ");Serial.println(t3,DEC);
        t2 = t4-t2;
        Serial.print("memcpy+ ");Serial.println(t2,DEC);
        t4 = t4-t1;
        Serial.print("elapsed ");Serial.println(t4,DEC);
        Serial.println(dst[3]);
        Serial.println(a[3]);
        Serial.println(c[3]);

        Serial.println();
	delay(3000);
}
