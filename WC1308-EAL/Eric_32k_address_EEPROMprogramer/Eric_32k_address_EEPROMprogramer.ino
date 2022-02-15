/*
 * BEN EATER VIDEO: https://www.youtube.com/watch?v=K88pgWhEb1M
 */

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define LATCH_SHIFT 4
#define EEPROM_D0 5
//#define EEPROM_D(1-6) (6-11)
#define EEPROM_D7 12
#define WRITE_EN 13

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80)); /*output enable by oring to control the bit*/
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(LATCH_SHIFT, LOW);
  digitalWrite(LATCH_SHIFT, HIGH);
  digitalWrite(LATCH_SHIFT, LOW);
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}


/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  delay(10);
}


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents() {
  for (word base = 0; base <= 32767; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%04x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

#define HLT 0b10000000000000000000000000000000  // Halt clock
#define MI  0b01000000000000000000000000000000  // Memory address register in
#define RI  0b00100000000000000000000000000000  // RAM data in
#define RO  0b00010000000000000000000000000000  // RAM data out
#define II  0b00001000000000000000000000000000  // Instruction register in
#define AI  0b00000100000000000000000000000000  // A register in
#define AO  0b00000010000000000000000000000000  // A register out
#define EO  0b00000001000000000000000000000000  // ALU out

#define SU  0b00000000100000000000000000000000  // ALU subtract
#define BI  0b00000000010000000000000000000000  // B register in
#define OI  0b00000000001000000000000000000000  // Output register in
#define CE  0b00000000000100000000000000000000  // Program counter enable
#define CO  0b00000000000010000000000000000000  // Program counter out
#define J   0b00000000000001000000000000000000  // Jump (program counter in)
#define FI  0b00000000000000100000000000000000  // Flags in
#define IDC 0b00000000000000010000000000000000  // Increment/Decrement

#define OFI 0b00000000000000001000000000000000  // Offset In
#define RAO 0b00000000000000000100000000000000  // Offset (Relative Address) Out
#define SCE 0b00000000000000000010000000000000  // Enable SP Count
#define DS  0b00000000000000000001000000000000  // Decrement SP
#define SO  0b00000000000000000000100000000000  // SP Out
#define SA  0b00000000000000000000010000000000  // Stack Segment Active
#define UNUSED  0b00000000000000000000010000000000
#define UCR 0b00000000000000000000000100000000  // microcode reset

#define DRS 0b00000000000000000001000000000000  // display register select (0 instruction/1 data)
#define DE  0b00000000000000000000001000000000  // display enable

//#define DRS 0b00000000000000000000000010000000  // display register select (0 instruction/1 data)
//#define DE  0b00000000000000000000000001000000  // display enable
 
#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC  0b01111
#define JZ  0b10000
#define JNC  0b10001
#define JNZ  0b10010


uint32_t UCODE[46][8] = {
  { MI|CO,  RO|II|CE,  UCR,      0,         0,           0,              0,       0, },   //  000000 - NOP
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|AI|UCR,   0,              0,       0, },   //  000001 - LDA
  { MI|CO,  RO|II|CE,  CO|MI,    RO|AI|CE|UCR,  0,       0,              0,       0, },   //  000010 - LAI
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|BI,       EO|AI|FI|UCR,   0,       0, },   //  000011 - ADD
  { MI|CO,  RO|II|CE,  CO|MI,    RO|BI|CE,  EO|AI|FI|UCR,0,              0,       0, },   //  000100 - ADI  
  { MI|CO,  RO|II|CE,  EO|AI|FI|UCR, 0,     0,           0,              0,       0, },   //  000101 - ADR
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|BI,       EO|AI|SU|FI|UCR,  0,     0, },   //  000110 - SUB
  { MI|CO,  RO|II|CE,  CO|MI,    RO|BI|CE,  EO|AI|FI|SU|UCR, 0,          0,       0, },   //  000111 - SUI
  { MI|CO,  RO|II|CE,  EO|AI|FI|SU|UCR, 0,    0,         0,              0,       0, },   //  001000 - SUR
  { MI|CO,  RO|II|CE,  IDC,      EO|AI|FI|IDC|UCR, 0,    0,              0,       0, },   //  001001 - INC
  { MI|CO,  RO|II|CE,  IDC,      EO|AI|FI|IDC|SU|UCR, 0, 0,              0,       0, },   //  001010 - DEC
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RI|AO,       UCR,            0,       0, },   //  001011 - STA
  { MI|CO,  RO|II|CE,  MI|CO,    RO|J,      UCR,         0,              0,       0, },   //  001100 - JMP
  { MI|CO,  RO|II|CE,  MI|CO,    CE,        UCR,         0,              0,       0, },   //  001101 - JC
  { MI|CO,  RO|II|CE,  MI|CO,    CE,        UCR,         0,              0,       0, },   //  001110 - JZ
  { MI|CO,  RO|II|CE,  MI|CO,    CE,        UCR,         0,              0,       0, },   //  001111 - JNC
  { MI|CO,  RO|II|CE,  MI|CO,    CE,        UCR,         0,              0,       0, },   //  010000 - JNZ
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|BI,       SU|FI|UCR,      0,       0, },   //  010001 - CMP
  { MI|CO,  RO|II|CE,  CO|MI,    RO|BI|CE,  FI|SU|UCR,   0,              0,       0, },   //  010010 - CPI
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|BI|UCR,   0,              0,       0, },   //  010011 - LDB
  { MI|CO,  RO|II|CE,  CO|MI,    RO|BI|CE|UCR,0,         0,              0,       0, },   //  010100 - LBI
  { MI|CO,  RO|II|CE,  CO|MI,    RO|DE|CE,    UCR,       0,              0,       0, },   //  010101 - DIN
  { MI|CO,  RO|II|CE,  CO|MI,    RO|DE|CE|DRS,UCR,       0,              0,       0, },   //  010110 - DSP
  { MI|CO,  RO|II|CE,  AO|DE|DRS,UCR,       0,           0,              0,       0, },   //  010111 - DSA
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|DE|DRS,   UCR,            0,       0, },   //  011000 - DSM
  { MI|CO,  RO|II|CE,  0,        0,         0,           0,              0,       0, },   //  011001
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OI|CE|UCR,0,         0,              0,       0, },   //  011010 - OPI
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|OI|UCR,   0,              0,       0, },   //  011011 - OPM
  { MI|CO,  RO|II|CE,  AO|OI|UCR,0,         0,           0,              0,       0, },   //  011100 - OUT
  { MI|CO,  RO|II|CE,  HLT,      0,         0,           0,              0,       0, },   //  011101 - HLT
  { MI|CO,  RO|II|CE,  CO|MI,    MI|RO|CE,  RO|AI|DS|SCE,0,           SO|MI, RI|AO|SA},   //  011110 - PUM
  { MI|CO,  RO|II|CE,  CO|MI,    RO|AI|CE|DS|SCE, 0,    SO|MI,           RI|AO|SA, 0,},   //  011111 - PUI
  { MI|CO,  RO|II|CE,  DS|SCE,   0,         SO|MI,      RI|AO|SA,        UCR,     0, },   //  100000 - PUA
  { MI|CO,  RO|II|CE,  DS|SCE,   UCR,       0,          0,               0,       0, },   //  100001 - PUN
  { MI|CO,  RO|II|CE,  SO|MI,    RO|AI|SA|SCE, CO|MI,   MI|RO|CE,        RI|AO   ,0, },   //  100010 - POM
  { MI|CO,  RO|II|CE,  SO|MI,    RO|AI|SA|SCE|UCR, 0,   0,               0,       0, },   //  100011 - POA
  { MI|CO,  RO|II|CE,  SCE|UCR,  0,           0,        0,               0,       0, },   //  100100 - PON
  { MI|CO,  RO|II|CE,  DS|SCE,   0,           SO|MI,    RI|CO|SA,        MI|CO,   RO|J,}, //  100101 - CAL
  { MI|CO,  RO|II|CE,  SO|MI,    RO|J|SA|SCE, CE|UCR,   0,               0,       0, },   //  100110 - RET
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|AI|SA|UCR,    0,       0, },   //  100111 - LAS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|BI|SA,        EO|AI|FI|UCR, 0, },   //  101000 - ADS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|BI|SA,        EO|AI|FI|SU|UCR, 0, },//  101001 - SUS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|BI|SA,        FI|SU|UCR,0,},   //  101010 - CPS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|BI|SA|UCR,    0,       0, },   //  101011 - LBS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   RO|OI|SA|UCR,    0,       0, },   //  101100 - OPS
  { MI|CO,  RO|II|CE,  CO|MI,    RO|OFI|CE,   RAO|MI,   AO|RI|SA,        UCR,     0, },   //  101101 - SAS  
  
};

/*uint16_t ucode[4][32][6];

void initUCode() {
  // ZF = 0, CF = 0
  memcpy_P(ucode[FLAGS_Z0C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z0C0][JNC][3] = RO|J;
  ucode[FLAGS_Z0C0][JNZ][3] = RO|J;
  // ZF = 0, CF = 1
  memcpy_P(ucode[FLAGS_Z0C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z0C1][JC][3] = RO|J;
  ucode[FLAGS_Z0C1][JNZ][3] = RO|J;

  // ZF = 1, CF = 0
  memcpy_P(ucode[FLAGS_Z1C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C0][JZ][3] = RO|J;
  ucode[FLAGS_Z1C0][JNC][3] = RO|J;

  // ZF = 1, CF = 1
  memcpy_P(ucode[FLAGS_Z1C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C1][JC][3] = RO|J;
  ucode[FLAGS_Z1C1][JZ][3] = RO|J;
}
*/
void setup() {

  //initUCode();
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(LATCH_SHIFT, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);

// Program data bytes
  Serial.print("Programming EEPROM");

  for (uint32_t address = 0; address <= 32767; address += 1) {
    word flags       = (address & 0b110000000000000) >> 13;
    word byte_sel    = (address & 0b001100000000000) >> 11;
    word instruction = (address & 0b000000111111000) >> 3;
    word steap       = (address & 0b000000000000111) >> 0;
    word unwanted    = (address & 0b000011000000000) >> 9;


    if(flags == FLAGS_Z0C0){
      UCODE[JNC][3] = RO|J;
      UCODE[JNZ][3] = RO|J;
      UCODE[JC][3] = CE;
      UCODE[JZ][3] = CE;
    }
    else if(flags == FLAGS_Z0C1){
      UCODE[JC][3] = RO|J;
      UCODE[JNZ][3] = RO|J;
      UCODE[JNC][3] = CE;
      UCODE[JZ][3] = CE;
    }
    else if(flags == FLAGS_Z1C0){
      UCODE[JNC][3] = RO|J;
      UCODE[JZ][3] = RO|J;
      UCODE[JC][3] = CE;
      UCODE[JNZ][3] = CE;  
    }
    else if(flags == FLAGS_Z1C1){
      UCODE[JC][3] = RO|J;
      UCODE[JZ][3] = RO|J;
      UCODE[JNC][3] = CE;
      UCODE[JNZ][3] = CE;  
    }

    if (unwanted != 0b00) {
     // writeEEPROM(address, 0xff);
    }
    else if (byte_sel == 0b00){
      writeEEPROM(address, UCODE[instruction][steap] >> 24);
      //Serial.println(UCODE[instruction][steap] >> 24, BIN);
    }
    else if (byte_sel == 0b01){
      writeEEPROM(address, UCODE[instruction][steap] >> 16);
      //Serial.println(UCODE[instruction][steap] >> 16, BIN);
    }
    else if (byte_sel == 0b10){
      writeEEPROM(address, UCODE[instruction][steap] >> 8);
      //Serial.println(UCODE[instruction][steap] >> 8, BIN);
    }
    else if (byte_sel == 0b11) {
      writeEEPROM(address, UCODE[instruction][steap] >> 0);
    }

    if (address % 1024 == 0) {
      Serial.println(address);
    }
  }

  Serial.println(" done");




  // Read and print out the contents of the EERPROM
  Serial.println("Reading EEPROM");
  printContents();
}


void loop() {
  // put your main code here, to run repeatedly:

}
