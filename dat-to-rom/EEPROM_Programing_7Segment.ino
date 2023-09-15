////////////////////////////
// For the pins
#define latchClock 3     // RCLK
#define shiftClock 4     // SRCLK
#define dataPin    2     // SER
#define EEPROM_D0  5
#define EEPROM_D7  12
#define WRITE_EN   13

////////////////////////////
// For time interval
#define LOOP_INTERVAL 1000

////////////////////////////////////////
// Seven-segment display for digit 0-9
// ( Common Cathode )
byte digit[] = {
  0b1111110, // 0
  0b0110000, // 1
  0b1101101, // 2
  0b1111001, // 3
  0b0110011, // 4
  0b1011011, // 5
  0b1011111, // 6
  0b1110000, // 7
  0b1111111, // 8
  0b1111011  // 9
};

byte sign = 0b0000001;

void setAddress(int address, bool outputEnable) {
  shiftOut(dataPin, shiftClock, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(dataPin, shiftClock, MSBFIRST, address);

  digitalWrite(latchClock, LOW);
  digitalWrite(latchClock, HIGH);
  digitalWrite(latchClock, LOW);
}

byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; ++pin) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /* outputEnable */ true);
  
  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin--) {
    data = (data << 1) + digitalRead(pin);
  }
  
  return data;
}

byte writeEEPROM(int address, byte data) {
  // Very crucial to put this setAddress() before setting pins to OUTPUT,
  // since if we read beforehand then means outputEnable is LOW (active)
  // the Arduino and the chips both OUTPUTING if we not setAddress() first.
  // NOT GOOD AT ALL :( 
  setAddress(address, /* outputEnable */ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; ++pin) {
    pinMode(pin, OUTPUT);
  }
  
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; ++pin) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }

  digitalWrite(WRITE_EN, LOW);
  __asm__ __volatile__ (" nop \n \t"); // wait 65ns
  __asm__ __volatile__ (" nop \n \t"); // wait 65ns
  __asm__ __volatile__ (" nop \n \t"); // wait 65ns
  digitalWrite(WRITE_EN, HIGH);
  delay(5);
}

void writeContent() {
  // D10 == 0 - Unsigned Notation
  for (int number = 0; number <= 255; ++number) {
    // D1 - blank (no need for sign)
    writeEEPROM(number, 0);                               

    // D2 - 1st digit (100th place)
    if (number >= 100)
      writeEEPROM(number + 256, digit[number / 100]);        
    else
      writeEEPROM(number + 256, 0);

    // D3 - 2nd digit (10th place)
    if (number >= 10)
      writeEEPROM(number + 512, digit[(number / 10) % 10]);  
    else
      writeEEPROM(number + 512, 0);

    // D4 - 3rd digit (1st place)
    writeEEPROM(number + 768, digit[number % 10]);           
  }

  // D10 == 1 - Signed Notation
  for (int number = 0; number <= 127; ++number) {
    // D1 - for sign
    writeEEPROM(number + 1024, 0);

    // D2 - 1st digit (100th place)
    if (number >= 100)
      writeEEPROM(number + 1024 + 256, digit[number / 100]);        
    else
      writeEEPROM(number + 1024 + 256, 0);

    // D3 - 2nd digit (10th place)
    if (number >= 10)
      writeEEPROM(number + 1024 + 512, digit[(number / 10) % 10]);  
    else
      writeEEPROM(number + 1024 + 512, 0);
      
    // D4 - 3rd digit (1st place)
    writeEEPROM(number + 1024 + 768, digit[abs(number) % 10]);          
  }

  for (int number = -128; number < 0; ++number) {
    // D1 - for sign
    writeEEPROM(number + 1024 + 256, sign);

    // D2 - 1st digit (100th place)
    if (number <= -100)
      writeEEPROM(number + 1024 + 256 + 256, digit[abs(number) / 100]);
    else
      writeEEPROM(number + 1024 + 256 + 256, 0);

    // D3 - 2nd digit (10th place)
    if (number <= -10)
      writeEEPROM(number + 1024 + 512 + 256, digit[(abs(number) / 10) % 10]);  
    else
      writeEEPROM(number + 1024 + 512 + 256, 0);
      
    // D4 - 3rd digit (1st place)
    writeEEPROM(number + 1024 + 768 + 256, digit[abs(number) % 10]);          
  }
}

void printContent() {
  for (int base = 0; base <= 2047; base += 16) {
    byte data[16];
    for (int offset = 0; offset < 16; ++offset) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x",
      base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
      data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]
    );
    Serial.println(buf);
  }
}

void setup() {
  pinMode(latchClock, OUTPUT);
  pinMode(shiftClock, OUTPUT);
  pinMode(dataPin, OUTPUT);

  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);

  Serial.begin(57600);

  // Should not write for too much
  writeContent();
  printContent();
}

void loop() {
  // Left empty
}
