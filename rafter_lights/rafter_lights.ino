#include "Adafruit_NeoPixel.h"

#include <SPI.h>
#include "RF24.h"

#define STRIP_PIN 8

#define HEADER_BYTE 0xA5
#define NULL_CMD    0x00

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_BRG     Pixels are wired for BRG bitstream
Adafruit_NeoPixel strip = Adafruit_NeoPixel(146, STRIP_PIN, NEO_BRG + NEO_KHZ800);

enum pattern_state {
  PATTERN_OFF = 0,
  PATTERN_RED_PULSE,
  PATTERN_MAX,
  PATTERN_WARM_HIGH,
  PATTERN_WARM_MED,
  PATTERN_WARM_LOW,
  PATTERN_RAINBOW_FADE,
  PATTERN_RAINBOW_CYCLE,
  PATTERN_DOT_CHASE,
  PATTERN_STATE_MAX
};

int state = PATTERN_OFF;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 (enable) and 10 (chip select) */
RF24 radio(9,10);

// radio addresses; this device will be listener #1 and will listen to master #1
byte listener_radio_address[] = "1list";
byte master_radio_address[] = "1mast";

void setup() {
  Serial.begin(115200);
  
  // strip setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'  

  // radio setup
  radio.begin();

  // Set the PA Level low to prevent power supply related issues just in case,
  // and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, listener_radio_address);

  radio.startListening();
}

void solid(uint32_t c) {
  for(uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

bool contains_valid_command(byte *receive_buffer, int buffer_length) {
  byte xor_sum;
  byte checksum = 0;

  for (int i = 0; i < buffer_length - 2; ++i) {
    if (receive_buffer[i] == HEADER_BYTE) {
      xor_sum ^= receive_buffer[i];
      if (receive_buffer[i+1] == NULL_CMD) {
        xor_sum ^= receive_buffer[i+1];
        checksum = ~xor_sum;
        if (receive_buffer[i+2] == checksum) {
          return true;
        }
      }
    }
  }

  return false;
}

void init_pattern(int state) {
  switch (state) {
    case PATTERN_OFF:
      solid(0);
      break;
    case PATTERN_RED_PULSE:
      break;
    case PATTERN_MAX:
      solid(0xFFFFFF);
      break;
    case PATTERN_WARM_HIGH:
      solid(strip.Color(255,130,80));
      break;
    case PATTERN_WARM_MED:
      solid(strip.Color(200,100,50));
      break;
    case PATTERN_WARM_LOW:
      solid(strip.Color(60,30,20));
      break;
    case PATTERN_RAINBOW_FADE:
      break;
    case PATTERN_RAINBOW_CYCLE:
      break;
    case PATTERN_DOT_CHASE:
      break;
    default:
      break;
  }
  strip.show();  
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

byte triangle(byte phase) {
  phase = phase % 255;
  if(phase < 128)
  {
    return phase;
  }
  else
  {
    return 255 - phase;
  }
}

void run_red_pulse() {
  int t = millis();
  for (int i = 0; i < strip.numPixels(); ++i) {
    int red = 120 + 0.5*(triangle(0.02*t + 20*i));
    int blu = 0.06*(triangle(0.02*t + 20*i+127));
    int grn = 0.17*(triangle(0.02*t + 20*i+127));
    strip.setPixelColor(i, strip.Color(red, grn, blu));
  }
}

// all pixels are the same color
void run_rainbow_fade() {
  unsigned long t = millis();
  float cycle_time = 20000;
  float fraction_complete = (t % (long)cycle_time) / cycle_time;
  int wheel_position = (int)(256 * fraction_complete) & 255;
  uint32_t color = Wheel(wheel_position);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
}

void run_rainbow_cycle() {
  unsigned long t = millis();
  float cycle_time = 20000;
  float fraction_complete = (t % (long)cycle_time) / cycle_time;
  int base_wheel_position = 256 * fraction_complete;
  for (int i = 0; i < strip.numPixels(); i++) {
    uint32_t color = Wheel(((i * 256 / strip.numPixels()) + base_wheel_position) & 255);
    strip.setPixelColor(i, color);
  }
}

void run_dot_chase() {
  unsigned long t = millis();
  float cycle_time = 3000;
  float fraction_complete = (t % (long)cycle_time) / cycle_time;
  int dot_location = strip.numPixels() * fraction_complete;
  solid(0);
  strip.setPixelColor(dot_location, 255, 220, 200);
}

void run_pattern(int state) {
  switch (state) {
    case PATTERN_OFF:
      break;
    case PATTERN_RED_PULSE:
      run_red_pulse();
      break;
    case PATTERN_MAX:
      break;
    case PATTERN_WARM_HIGH:
      break;
    case PATTERN_WARM_MED:
      break;
    case PATTERN_WARM_LOW:
      break;
    case PATTERN_RAINBOW_FADE:
      run_rainbow_fade();
      break;
    case PATTERN_RAINBOW_CYCLE:
      run_rainbow_cycle();
      break;
    case PATTERN_DOT_CHASE:
      run_dot_chase();
      break;
    default:
      break;
  }
  strip.show();
}

void loop() {
  byte receive_buffer[32] = {0,};
  int bytes_received = 0;

  // run the pattern
  run_pattern(state);

  if (radio.available()) {
    radio.read(receive_buffer, sizeof(receive_buffer));
  }

  if (!contains_valid_command(receive_buffer, sizeof(receive_buffer))) {
    return;
  }

  state++;
  if (state >= PATTERN_STATE_MAX) {
    state = PATTERN_OFF;
  }

  // do any pattern initialization commands
  init_pattern(state);
  
  // only need to do updates like every 5ms
  delay(5);
}

