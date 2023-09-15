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

////////////////////////////
// For control signal
#define HLT 0b1000000000000000
#define MI  0b0100000000000000
#define RI  0b0010000000000000
#define RO  0b0001000000000000
#define IO  0b0000100000000000
#define II  0b0000010000000000
#define AO  0b0000001000000000
#define AI  0b0000000100000000
#define EO  0b0000000010000000
#define SUB 0b0000000001000000
#define BI  0b0000000000100000
#define OI  0b0000000000010000
#define CE  0b0000000000001000
#define J   0b0000000000000100
#define CO  0b0000000000000010
#define FI  0b0000000000000001

////////////////////////////
// For flags
#define FLAG_C0Z0 0b00
#define FLAG_C0Z1 0b01
#define FLAG_C1Z0 0b10
#define FLAG_C1Z1 0b11

/////////////////////////////
// For instructions
#define _NO  0b0000
#define _LDA 0b0001
#define _ADD 0b0010
#define _SUB 0b0011
#define _STA 0b0100
#define _LDI 0b0101
#define _JMP 0b0110
#define _JC  0b0111
#define _JZ  0b1000
#define _AEI 0b1001
#define _SEI 0b1010
#define _SHL 0b1011
#define _SLF 0b1101
#define _OUT 0b1110
#define _HLT 0b1111

const PROGMEM uint16_t UCODE_TEMPLATE[16][8] = {
  { MI|CO, RO|II|CE, 0,        0,        0,            0,            0, 0 }, // 0000 - NOP (do nothing)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|MI,    RO|AI,        0,            0, 0 }, // 0001 - LDA (load A register from RAM)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|MI,    RO|BI,        EO|AI|FI,     0, 0 }, // 0010 - ADD (add instruction)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|MI,    RO|BI,        EO|AI|SUB|FI, 0, 0 }, // 0011 - SUB (subtract)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|MI,    AO|RI,        0,            0, 0 }, // 0100 - STA (store A to RAM)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|AI,    0,            0,            0, 0 }, // 0101 - LDI (load effective immediately)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|J,     0,            0,            0, 0 }, // 0110 - JMP (jump)
  { MI|CO, RO|II|CE, CO|MI|CE, 0,        0,            0,            0, 0 }, // 0111 - JC  (jump carry)
  { MI|CO, RO|II|CE, CO|MI|CE, 0,        0,            0,            0, 0 }, // 1000 - JZ  (jump zero)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|BI,    EO|AI|FI,     0,            0, 0 }, // 1001 - AEI (add effective immediately)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|BI,    EO|AI|SUB|FI, 0,            0, 0 }, // 1010 - SEI (subtract effective immediately)
  { MI|CO, RO|II|CE, CO|MI|CE, RO|MI,    RO|AI|BI,     EO|AI|FI,     0, 0 }, // 1011 - SHL (shift left)
  { MI|CO, RO|II|CE, 0,        0,        0,            0,            0, 0 }, // 1100
  { MI|CO, RO|II|CE, AO|BI,    EO|AI|FI, 0,            0,            0, 0 }, // 1101 - SLF (shift left FAST)
  { MI|CO, RO|II|CE, AO|OI,    0,        0,            0,            0, 0 }, // 1110 - OUT (output content of A register)
  { MI|CO, RO|II|CE, HLT,      0,        0,            0,            0, 0 }, // 1111 - HLT (halt the clock)
};

uint16_t ucode[4][16][8];
void createUCodeFromTemplate() {
  // CF = 0, ZF = 0
  memcpy_P(ucode[FLAG_C0Z0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));

  // CF = 0, ZF = 1
  memcpy_P(ucode[FLAG_C0Z1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAG_C0Z1][_JZ][3] = RO|J;

  // CF = 1, ZF = 0
  memcpy_P(ucode[FLAG_C1Z0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAG_C1Z0][_JC][3] = RO|J;

  // CF = 1, ZF = 1
  memcpy_P(ucode[FLAG_C1Z1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAG_C0Z1][_JZ][3] = RO|J;
  ucode[FLAG_C1Z0][_JC][3] = RO|J;
}

/*
 * Converting Ben Eater's address format into my own format
 *  == Ben Eater ==
 *   | Left empty || Flags  | opcode || Step |
 *         00         00       0000     000   <- Easier to use
 *
 *  ==== Mine =====
 *   | Step || Flags || Left empty || opcode |
 *     000      00          00         0000   <- What the hell was I thinking...
 */
int formatAddress(int original) {
  return 0 |
  ((original & 0b00000000111) << 8) | /* Step */
  ((original & 0b00001111000) >> 3) | /* Op code */
  ((original & 0b00110000000) >> 1);  /* Flags */
}

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

  for (int pin = EEPROM_D7; pin >= EEPROM_D0; --pin) {
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

/*
 * Put everything you want to write to EEPROM in here.
 */
void writeContent() {
  Serial.print("Programming your EEPROM");
  int flag, instruction, cycle, address;
  for (flag = FLAG_C0Z0; flag <= FLAG_C1Z1; ++flag) {
    for (instruction = 0b0000; instruction <= 0b1111; ++instruction) {
      for (cycle = 0b000; cycle <= 0b111; ++cycle) {
        address = cycle + instruction * 8 + flag * 16 * 8;
        //writeEEPROM(formatAddress(address), ucode[flag][instruction][cycle] >> 8);
        writeEEPROM(formatAddress(address), ucode[flag][instruction][cycle]);
      }
      if (instruction % 3) Serial.print(".");
    }
  }
  Serial.println(" done.");
}

/*
 * Erase entire EEPROM
 */
void eraseEEPROM() {
  Serial.print("Erasing EEPROM");
  for (int address = 0; address < 2048; ++address) {
    writeEEPROM(address, 0xff);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done.");
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
  createUCodeFromTemplate();
  //eraseEEPROM();
  writeContent();
  printContent();
}

void loop() {
  // Left empty
}
