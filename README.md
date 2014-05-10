# DUE Zoo
<br>https://github.com/manitou48/DUEZoo


Collection of various DUE sketches and results.

Files | Description
---|---
chiptemp.ino  | read DUE internal analog temperature sensor
dmaspi.ino  |  unconnected SPI test at varous MHz and with DMA
mem2mem.ino  | memcpy vs DMA memory-to-memory
mem2mem2.ino | memcpy dueling DMA memory-to-memory with IRQ
**power.txt**    | board current draw with board LED off and on, AVR low power
tone1.ino    |proof-of-concept tone() with timer with IRQ
rng.ino      | display or transmit DUE's hardware random number generator, looks good with diehard, ent, assess
maplerng.pde  | maple LSI timer/systick random number generator
**RNGperf.txt**  | RNG performance, DUE, teensy, UNO, Raspberry PI, maple
W5100.cpp .h |modifications to support w5200 wiznet chip on DUE replace in hardware/arduino/sam/libraries/Ethernet/utility/ also Ethernet.h has a simple modification
w5100.cpp.dma1|SPI+DMA DUE version for w5200
**perf.txt**      |simple computational benchmarks
**I2Cperf.txt**   |I2C performance  100KHz and 400KHz
**SDperf.txt**    |SD/SPI performance at various clock rates and with DMA
**SPIperf.txt**   |SPI performance at various clock rates and with DMA
**mem2mem.txt**   |timing results for memcpy()/memset() and DMA versions
**wizperf.txt**   |Ethernet performance of wiznet WIZ820io that uses W5200 chip SPI and SPI+DMA results for DUE, teensy 3, and maple plus UNO W5100 performance for UNO
IRtest/       |DUE IRremote proof of concept
**isrperf.txt**   |interrupt latency results for DUE, maple, teensy
isrperf.ino   |DUE sketch to measure interrupt latency
**cc3000.txt**   |wifi performance of TI CC3000 chip

<pre>
---------------------- details --------------------

Changing the DUE's I2C clock rate TWI_CLOCK in 
   arduino-1.5/hardware/arduino/sam/libraries/Wire/Wire.h 
requires restarting the IDE.  TWI speed for teensy 3.0 in Wire.ccp,
must restart IDE.
</pre>


Results and sketches of testing various crystals, resonators, RC oscillators,
and TCXOs  with various MCUs 
( UNO, DUE, maple, teensy 3.0,  propeller, beagle, raspberry pi) see
  https://github.com/manitou48/crystals

<p>



<i>“One accurate measurement is worth a thousand expert opinions”</i>
<br>
  -- Adm Grace Hopper ( Dec 9 1906 to Jan 1 1992)
