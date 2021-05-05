int inByte; // Stores incoming command

struct tLine {
  int address;
  int data;
};

tLine * program;

void squirt(tLine * program, int length)
{
  // loop over the array and manipulate digital lines
  int i;
  for (i=0; i<length; ++i) {
    
  }
}

void setup() {
Serial.begin(9600);
pinMode(13, OUTPUT); // Led pin
Serial.println("Ready"); // Ready to receive commands
}

void loop() {

  int length;
  tLine line;
  int i;
  
  if (Serial.available() > 0) { // A byte is ready to receive

    length = Serial.read();

//    length = length - '0';

    program = (tLine*)malloc(length * sizeof(tLine));
    
    for (i=0; i<length; ++i) {

        while (Serial.available() < 2) {
//          Serial.println("wait");  
        } // busy wait until data avalable

        program[i].address = Serial.read();
        program[i].data = Serial.read();

//        Serial.print(address);
//        Serial.print(data);
      
    } // end of for loop

    // print out array
    for (i=0; i<length; ++i) {
      Serial.write(program[i].address);
      Serial.write(program[i].data); 
    }
    Serial.flush();

    squirt(program, length);
    
    free(program);
  }
}
