struct tLine {
  int address;
  int data;
};

tLine * program;


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
  delay(1000);
}

void programRAM(tLine * program, int length) {
  for (int j = 2; j <=13; j += 1){
    pinMode(j, OUTPUT);
  }
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(LATCH_SHIFT, OUTPUT);
  digitalWrite(LATCH_SHIFT, LOW);
  digitalWrite(WRITE, HIGH);
  pinMode(WRITE, OUTPUT);

  for (int i = 0; i < length; i += 1) {
    writeRAM(program[i].address, program[i].data);
  }

  done();
  //disable arduino control for dip switches
  for (int j = 2; j <=13; j += 1){
    pinMode(j, INPUT);
  }
  digitalWrite(LATCH_SHIFT, HIGH); //latch shift here acts to disable outputs at end of program. We can do this because it is always low when it needs to be (output enabled)

}

void setup() {
  
Serial.begin(9600);
pinMode(13, OUTPUT); // Led pin
}

void loop() {
  
  int length;
  tLine line;
  int i;
  
  if (Serial.available() > 0) { // A byte is ready to receive

    length = Serial.read();

    program = (tLine*)malloc(length * sizeof(tLine));
    
    for (i=0; i<length; ++i) {

        while (Serial.available() < 2) {  
        } // busy wait until data avalable

        program[i].address = Serial.read();
        program[i].data = Serial.read();
    } // end of for loop

    // print out array
    //for (i=0; i<length; ++i) {
    //  Serial.write(program[i].address);
    //  Serial.write(program[i].data); 
    //}
    //Serial.flush();

    programRAM(program,length);
    
    free(program);
  }
}
