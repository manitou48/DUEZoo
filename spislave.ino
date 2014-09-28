// SPI slave for DUE, hack
//  master will lower CS and manage CLK
// MISO to MISO  MOSI to MOSI   CLK to CLK   CS to CS, common ground
//   http://forum.arduino.cc/index.php/topic,157203.0.html
// core hardware/arduino/sam/system/libsam/source/spi.c
// TODO:  C class, interrupt, FIFO/16-bit, DMA?

#include <SPI.h>

// assumes MSB
static BitOrder bitOrder = MSBFIRST;

void slaveBegin(uint8_t _pin) {
	SPI.begin(_pin);
	REG_SPI0_CR = SPI_CR_SWRST;     // reset SPI
	REG_SPI0_CR = SPI_CR_SPIEN;     // enable SPI
	REG_SPI0_MR = SPI_MR_MODFDIS;     // slave and no modefault
	REG_SPI0_CSR = SPI_MODE0;    // DLYBCT=0, DLYBS=0, SCBR=0, 8 bit transfer
}


byte transfer(uint8_t _pin, uint8_t _data) {
    // Reverse bit order
    if (bitOrder == LSBFIRST)
        _data = __REV(__RBIT(_data));
    uint32_t d = _data;

    while ((REG_SPI0_SR & SPI_SR_TDRE) == 0) ;
    REG_SPI0_TDR = d;

    while ((REG_SPI0_SR & SPI_SR_RDRF) == 0) ;
    d = REG_SPI0_RDR;
    // Reverse bit order
    if (bitOrder == LSBFIRST)
        d = __REV(__RBIT(d));
    return d & 0xFF;
}

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

void prregs() {
	Serial.begin(9600);
	while(!Serial);
	PRREG(REG_SPI0_MR);
	PRREG(REG_SPI0_CSR);
	PRREG(REG_SPI0_SR);
}


#define SS 10
void setup() {
	slaveBegin(SS);
	prregs();  // debug
}

void loop() {
	byte in;
	static byte out=0x83;

	in = transfer(SS, out);
	out = in;
}
