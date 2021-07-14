
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define LATCH_SHIFT 4 //also acts to disable outputs at end of program 
#define ADDRESS_0 5
//#define ADDRESS_(1-6) (6-11)
#define ADDRESS_7 12
#define WRITE 13

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setData(byte data) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, data);
  Serial.println(data);
  digitalWrite(LATCH_SHIFT, LOW);
  digitalWrite(LATCH_SHIFT, HIGH);
  digitalWrite(LATCH_SHIFT, LOW);
}


void done(){
    for (int pin = ADDRESS_0; pin <= ADDRESS_7; pin += 1) {
    pinMode(pin, OUTPUT);
  }
  for(int i = 0; i<=4; i +=1){  
    for (int pin = ADDRESS_0; pin <= ADDRESS_7; pin += 1) {
     int address = 0b00000000;
     digitalWrite(pin, address & 1);
     address = address >> 1;
    }
    delay(100);
    for (int pin = ADDRESS_0; pin <= ADDRESS_7; pin += 1) {
     int address = 0b11111111;
     digitalWrite(pin, address & 1);
     address = address >> 1;
    }
    delay(100);
  }
}

/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeRAM(byte address, byte data) {
  setData(data);
  
  for (int pin = ADDRESS_0; pin <= ADDRESS_7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = ADDRESS_0; pin <= ADDRESS_7; pin += 1) {
    digitalWrite(pin, address & 1);
    address = address >> 1;
  }
  digitalWrite(WRITE, LOW);
  delayMicroseconds(10);
  digitalWrite(WRITE, HIGH);
  Serial.println("WROTE");
  delay(100);
}

#define NOP 0b00000000
#define LDA 0b00000001
#define LAI 0b00000010
#define ADD 0b00000011
#define ADI 0b00000100
#define ADR 0b00000101
#define SUB 0b00000111
#define SUI 0b00001000
#define SUR 0b00001001
#define INC 0b00001011
#define DECR 0b00001100
#define STA 0b00001101
#define JMP 0b00001110
#define JC  0b00001111
#define JZ  0b00010000
#define JNC 0b00010001
#define JNZ 0b00010010
#define CMP 0b00010011
#define CPI 0b00010100
#define LDB 0b00010110
#define LBI 0b00010110

#define DSP 0b00011100
#define OPM 0b00011101
#define OUT 0b00011110
#define HLT 0b00011111




byte fib[24][2]{
  {0x00, LAI},
  {0x01, 0b00000001},
  {0x02, STA},
  {0x03, 0b11111111},
  {0x04, LAI},
  {0x05, 0b00000000},
  {0x06, STA},
  {0x07, 0b11111110},
  {0x08, OUT},
  {0x09, LDA},
  {0x0A, 0b11111111},
  {0x0B, ADD},
  {0x0C, 0b11111110},
  {0x0D, STA},
  {0x0E, 0b11111111},
  {0x0F, OUT},
  {0x10, LDA},
  {0x11, 0b11111110},
  {0x12, ADD},
  {0x13, 0b11111111},
  {0x14, JC},
  {0x15, 0b00000000},
  {0x16, JMP},
  {0x17, 0b00000110}
};

byte countUpDown[16][2]{
  {0x00, LDA},
  {0x01, 0b11111111},
  {0x02, OUT},
  {0x03, INC},
  {0x04, CMP},
  {0x05, 0b11111110},
  {0x06, JNZ},
  {0x07, 0b00000010},
  {0x08, OUT},
  {0x09, DECR},
  {0x0A, CMP},
  {0x0B, 0b11111111},
  {0x0C, JZ},
  {0x0D, 0b00000010},
  {0x0E, JMP},
  {0x0F, 0b00001000},
};

byte x = 255;
byte y = 50;

byte mult[30][2]{
  {0x00, LAI},
  {0x01, 0b00000000},
  {0x02, STA},
  {0x03, 0b11111101},
  {0x04, LDA},
  {0x05, 0b11111111},
  {0x06, STA},
  {0x07, 0b11111100},
  
  {0x08, LDA},
  {0x09, 0b11111100},
  {0x0A, CPI},
  {0x0B, 0b00000000},
  {0x0C, JZ},
  {0x0D, 0b10000000},
  {0x0E, DECR},
  {0x0F, STA},
  {0x10, 0b11111100},
  
  {0x11, LDA},
  {0x12, 0b11111101},
  {0x13, ADD},
  {0x14, 0b11111110},
  {0x15, STA},
  {0x16, 0b11111101},
  {0x17, JMP},
  {0x18, 0b00001000},

  {0x80, OPM},
  {0x81, 0b11111101},
  {0x82, HLT},

  {0xFE, x},
  {0xFF, y}

  

//252 is decrementer
//253 is product
//254 is x
//255 is y
};

byte divide[37][2]{
  {0x00, LAI},
  {0x01, 0b00000000},
  {0x02, STA},
  {0x03, 0b11111101},
  
  {0x04, LDA},
  {0x05, 0b11111111},
  {0x06, CPI},
  {0x07, 0b00000000},
  {0x08, JZ},
  {0x09, 0b11000000},

  {0x0A, LDA},
  {0x0B, 0b11111110},
  {0x0C, CMP},
  {0x0D, 0b11111111},
  {0x0E, JC},
  {0x0F, 0b10000000},
  
  {0x10, SUR},
  {0x11, STA},
  {0x12, 0b11111110},
  
  {0x13, LDA},
  {0x14, 0b11111101},
  {0x15, INC},
  {0x16, STA},
  {0x17, 0b11111101},
  
  {0x18, JMP},
  {0x19, 0b00001010},

  {0x80, OPM},
  {0x81, 0b11111101},
  {0x82, HLT},

  {0xC0, DSP},
  {0xC1, 0b11111111},
  {0xC2, DSP},
  {0xC3, 0b00000000},
  {0xC4, JMP},
  {0xC5, 0xC0},

  {0xFE, x},
  {0xFF, y}

//253 is incrementer/quotient
//254 is x
//255 is y
};



#define prog fib

void setup() {

  //initUCode();
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(LATCH_SHIFT, OUTPUT);
  digitalWrite(WRITE, HIGH);
  pinMode(WRITE, OUTPUT);
  Serial.begin(57600);

// Program data bytes
  Serial.print("Programming RAM");

  for (int i = 0; i < sizeof(prog)/sizeof(prog[0]); i += 1) {
    writeRAM(prog[i][0], prog[i][1]);
  }

  Serial.println(" done");

  done();
  //disable arduino control for dip switches
  for (int j = 2; j <=13; j += 1){
    pinMode(j, INPUT);
  }
  digitalWrite(LATCH_SHIFT, HIGH); //latch shift here acts to disable outputs at end of program. We can do this because it is always low when it needs to be (output enabled)

}

  
void loop() {
  // put your main code here, to run repeatedly:

}
