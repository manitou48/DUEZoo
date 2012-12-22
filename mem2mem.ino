/**********************************************************************
 use DMA for memcpy memset
 DUE AHB DMA (ch 24) has 6 channels, 3/5 have 32B FIFO (others 8)
 ASAP_CFG faster than ALAP_CFG
 */


// mem2mem works on all channels
#define DMAC_MEMCH 5

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
  while (!dmac_channel_transfer_done(DMAC_MEMCH)) {}
  dmac_channel_disable(DMAC_MEMCH);   // needed?  SOD does this?

}

void memset32(uint32_t *dst, uint32_t word, uint32_t n) {
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
  while (!dmac_channel_transfer_done(DMAC_MEMCH)) {}
  dmac_channel_disable(DMAC_MEMCH);   // needed?  SOD does this?
}

#define WORDS 1000
uint32_t src[WORDS],dst[WORDS];

void setup(){
	Serial.begin(9600);
	pmc_enable_periph_clk(ID_DMAC);
	dmac_disable();
	 DMAC->DMAC_GCFG = DMAC_GCFG_ARB_CFG_FIXED;
	dmac_enable();
}

void loop(){
	int i,t1,t2;
	
	for (i=0;i<WORDS;i++){
		dst[i]=0;
		src[i]=i;
	}
	memcpy32(dst,src,WORDS);
	Serial.println(dst[3],DEC);
        memset32(dst,45,WORDS);
        Serial.println(dst[3],DEC);
        t1=micros();
        memcpy32(dst,src,WORDS);
        t2 = micros() - t1;
        Serial.print("memcpy32 ");Serial.println(t2,DEC);
        t1=micros();
        for(i=0;i<WORDS;i++) dst[i] = src[i];
        t2 = micros() - t1;
        Serial.print("loop ");Serial.println(t2,DEC);
        
        t1=micros();
        memset32(dst,66,WORDS);
        t2 = micros() - t1;
        Serial.print("memset32 ");Serial.println(t2,DEC);
        t1=micros();
        for(i=0;i<WORDS;i++) dst[i] = 66;
        t2 = micros() - t1;
        Serial.print("loop ");Serial.println(t2,DEC);
        
        dst[3]=99;
        t1=micros();
        memcpy(dst,src,4*WORDS);
        t2 = micros() - t1;
        Serial.print("memcpy ");Serial.println(t2,DEC);
        Serial.println(dst[3],DEC);
        t1=micros();
        memset(dst,66,4*WORDS);
        t2 = micros() - t1;
        Serial.print("memset ");Serial.println(t2,DEC);
        Serial.println(dst[3],HEX);
        Serial.println();
	delay(3000);
}
