//http://arduino.cc/forum/index.php/topic,129013.0.html
float trans = 3.3/4096;
float offset = 0.8;
float factor = 0.00256;
int fixtemp = 25;

void setup() {
  Serial.begin(9600);
}

void loop0() {
  // try just analogRead  didn't work just 0s
  char str[32];
  adc_enable_channel(ADC, ADC_TEMPERATURE_SENSOR);
  adc_enable_ts(ADC);
  analogReadResolution(12);
  while(1) {
    int ain = analogRead(15);
  float v = trans * ain;

  float c = fixtemp + (v - offset ) / factor;
  float f = 1.8*c +32;
  
  sprintf(str,"%d %.3fv %.2fC %.1fF",ain,v,c,f);
  Serial.println(str);
  delay(1000);
  }
}

void loop() {
  char str[32];
  int ain = temperatur();
  float v = trans * ain;

  float c = fixtemp + (v - offset ) / factor;
  float f = 1.8*c +32;
  
  sprintf(str,"%d %.3fv %.2fC %.1fF",ain,v,c,f);
  Serial.println(str);
  delay(1000);
}

uint32_t temperatur() {
  uint32_t ulValue = 0;
  uint32_t ulChannel;
  
  // Enable the corresponding channel
  adc_enable_channel(ADC, ADC_TEMPERATURE_SENSOR);

  // Enable the temperature sensor
  adc_enable_ts(ADC);

  // Start the ADC
  adc_start(ADC);

  // Wait for end of conversion
  while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY);

  // Read the value
  ulValue = adc_get_latest_value(ADC);

  // Disable the corresponding channel
  adc_disable_channel(ADC, ADC_TEMPERATURE_SENSOR);

  return ulValue;
}
