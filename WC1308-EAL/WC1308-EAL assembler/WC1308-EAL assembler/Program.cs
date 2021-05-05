using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Net.Sockets;
using System.Net;

namespace WC1308_EAL_Assembler {
    class Program {

        enum protocols {
            TCP,
            serial
        }

        static void Main(string[] args) {

            //declare vars
            string readFilePath;
            protocols protocol;

            //parse cmd line args
            try {

                //if --help exists show list of comands and break
                if (Array.Exists(args, element => element.ToUpper() == "--HELP")) {
                    Console.WriteLine("USEAGE");
                    Console.WriteLine("Required Params");
                    Console.WriteLine("--r    [protocol]    \"COM\" for serial protocol or \"TCP\" for wifi TCP protocol");
                    Console.WriteLine("--p    [port]        COM port (serial) or TCP port (wifi)");
                    Console.WriteLine("--f    [path]        program .txt file path");
                    Console.WriteLine("Optional Params");
                    Console.WriteLine("--help               shows this menu");
                    Console.WriteLine("--COM                shows avaliable COM ports menu");
                    Console.WriteLine("--bin                output to binary file");
                    Console.WriteLine("--v                  verbose");
                    return;
                }
                //if --v exists enable verbose output
                if (Array.Exists(args, element => element.ToUpper() == "--V")) {
                    verbose = true;
                }
                //if --COM exists show list of valid COM ports and break
                if (Array.Exists(args, element => element.ToUpper() == "--COM")) {
                    Console.WriteLine("Available Ports:");
                    foreach (string s in SerialPort.GetPortNames()) {
                        Console.WriteLine("   {0}", s);
                    }
                    return;
                }
                //if --p exists assign arg to port, else throw error
                if (Array.Exists(args, element => element.ToUpper() == "--P")) {
                    port = args[Array.FindIndex(args, element => element.ToUpper() == "--P") + 1];
                }
                else {
                    throw new IOException("No port specified");
                }
                //if --f exists assign arg to file path, else throw error
                if (Array.Exists(args, element => element.ToUpper() == "--F")) {
                    readFilePath = args[Array.FindIndex(args, element => element.ToUpper() == "--F") + 1];
                }
                else {
                    throw new FileNotFoundException("No file path specified");
                }
                //if --r exists assign arg to protocol type, else throw error
                if (Array.Exists(args, element => element.ToUpper() == "--R")) {
                    protocol = (protocols)protocols.Parse(typeof(protocols),args[Array.FindIndex(args, element => element.ToUpper() == "--R") + 1]);
                }
                else {
                    throw new IOException("No protocol specified");
                }


                //read file into array of lines and clean
                primeForAssembly(readFilePath);
                //put labels and corespnding addresses in a dictionary
                parseLabels();
                //parse text into binary array
                assemble();
                //send assembled bytes over desired protocol
                switch (protocol) {
                    case protocols.serial:
                        serialOut();
                        break;
                    case protocols.TCP:
                        wifiOut();
                        break;
                    default:
                        throw new IOException("Invalid protocol:" + protocol.ToString());
                }

                //if --bin exists output assembled byte array to a file as well
                if (Array.Exists(args, element => element.ToUpper() == "--BIN")) {
                    string writeFilePath = readFilePath.Remove(readFilePath.Length - 3, 3) + "bin";
                    writeBinFile(writeFilePath);

                }
            }
            //catching errors
            catch (FileNotFoundException e) {
                Console.WriteLine("FileNotFoundException: " + e.Message);
            }
            catch (IOException e) {
                Console.WriteLine("IOException: " + e.Message);
            }
            catch (UnknownLabelException e) {
                Console.WriteLine("UnknownLabelException: " + e.Message);
            }
            catch (UnknownOperandException e) {
                Console.WriteLine("UnknownOperandException: " + e.Message);
            }
            //catch (Exception e) {
            //    Console.WriteLine("Unhandeled Exception: " + e.Message);
            //}
        }

        //program class vars
        static string port;
        static bool verbose = false;

        static List<string> lines = new List<string>();
        static byte linesInFile = 0;

        static Dictionary<string, byte> Labels = new Dictionary<string, byte>();
        static byte _base = 2;

        static List<byte> assembled = new List<byte>();

        //takes in a file path, updates list of lines and linesInFile
        static void primeForAssembly(string filePath) {

            string line;
            //open file, get lines in file, clean up lines and put them into a list
            using (StreamReader sr = File.OpenText(filePath)) {

                if (verbose) Console.WriteLine("file opened at " + filePath);

                while ((line = sr.ReadLine()) != null) {
                    lines.Add(line);
                    ++linesInFile;
                }
                for (int i = 0; i < linesInFile; ++i) {
                    lines[i] = lines[i].Replace("\n", string.Empty).Replace("\r", string.Empty);
                }
            }
            if (verbose) Console.WriteLine("file closed at " + filePath);
        }


        //updates dictonary with label/address pairs
        static void parseLabels() {

            byte nextAddress = 0;

            //iterate through all lines in the file
            for (int i = 0; i < linesInFile; ++i) {
                
                //split line on white space, seperates opcode from operand
                string[] token = lines[i].Split(new char[] { ' ','\t' });

                //if first "opcode" specifies base metadata and is at line 0, change the base encoding mode and continue
                if (token[0] == ";base") {
                    if (i == 0) {
                        _base = Convert.ToByte(token[1], 10);
                        if (verbose) Console.WriteLine("base encoding mode: " + _base);
                        continue;
                    }
                    else {
                        Console.WriteLine("WARNING: invalid base specification at address line " + i);
                        continue;
                    }
                }
                //......FEATURE NOT FULLY ADDED YET......//
                //if specifying adress for a label (or other code), hard update next adress
                //TODO: also need to update in sending array
                if(token[0] == ";addr") {
                    nextAddress = Convert.ToByte(token[1],_base);
                    continue;
                }
                //......FEATURE NOT FULLY ADDED YET......//

                //skip over comments
                if (lines[i][0] == '#') {
                    continue;
                }
                //if first character specifies label, add label and nextAddress (the address that the label is pointing too) to the dictonary as a key data pair
                else if (lines[i][0] == '.') {
                    string label = lines[i].Replace(".", String.Empty);
                    Labels.Add(label, (byte)(nextAddress));
                }
                //if the line is an opcode that takes up one line in RAM (only opcode no operand), add one to the nextAddress
                else if (token[0] == "NOP" || token[0] == "OUT" || token[0] == "ADR" || token[0] == "SUR" || token[0] == "INC" || token[0] == "DEC" || token[0] == "HLT") {
                    ++nextAddress;
                }
                //if the line is an opcode that takes up two lines in RAM (opcode and operand), add two to the nextAddress
                else {
                    nextAddress += 2;
                }
            }
            if (verbose) Console.WriteLine("labels parsed");
        }

        
        //parses all opcodes and operands into binary and puts them into assembled byte list sequentally
        static void assemble() {

            //iterate through all lines in the file
            for (int i = 0; i < linesInFile; ++i) {

                //split line on white space, seperates opcode from operand
                string[] token = lines[i].Split(new char[] { ' ', '\t' });

                //don't parse labels, coments, or metadata
                if (lines[i][0] == '.' || lines[i][0] == '#' || lines[i][0] == ';') {
                    continue;
                }

                //Determine the opcode (token[0]) and add corect bytes to list accordingly

                //if it is a one line instruction (no operand), add its binary opcode to the assembled list
                if (token[0].ToUpper() == "NOP") {
                    assembled.Add(0b00000000);
                }
                //if it is a two line instruction (opcode and operand), add its binary opcode and the operand token[1] to the assembled list
                else if (token[0].ToUpper() == "LDA") {
                    assembled.Add(0b00000001);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "LAI") {
                    assembled.Add(0b00000010);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "ADD") {
                    assembled.Add(0b00000011);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "ADI") {
                    assembled.Add(0b00000100);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "ADR") {
                    assembled.Add(0b00000101);
                }
                else if (token[0].ToUpper() == "SUB") {
                    assembled.Add(0b00000111);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "SUI") {
                    assembled.Add(0b00001000);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "SUR") {
                    assembled.Add(0b00001001);
                }
                else if (token[0].ToUpper() == "INC") {
                    assembled.Add(0b00001011);
                }
                else if (token[0].ToUpper() == "DEC") {
                    assembled.Add(0b00001100);
                }
                else if (token[0].ToUpper() == "STA") {
                    assembled.Add(0b00001101);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                //if it is a jump instuction, add binary opcode to the assembled array
                //Then look up the label/operand in the labels dictonary and add that byte to the list
                //If the label dosen't exist as a key in the dictonary throw an error
                else if (token[0].ToUpper() == "JMP") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00001110);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        throw new UnknownLabelException("Unknown Label " + token[1] + " at address line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JC") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00001111);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        throw new UnknownLabelException("Unknown Label " + token[1] + " at address line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JZ") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010000);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        throw new UnknownLabelException("Unknown Label " + token[1] + " at address line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JNC") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010001);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        throw new UnknownLabelException("Unknown Label " + token[1] + " at address line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JNZ") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010010);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        throw new UnknownLabelException("Unknown Label " + token[1] + " at address line " + i);
                    }
                }
                else if (token[0].ToUpper() == "CMP") {
                    assembled.Add(0b00010011);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "CPI") {
                    assembled.Add(0b00010100);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "LDB") {
                    assembled.Add(0b00010101);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "LBI") {
                    assembled.Add(0b00010110);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "DSP") {
                    assembled.Add(0b00011100);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "OPM") {
                    assembled.Add(0b00011101);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "OUT") {
                    assembled.Add(0b00011110);
                }
                else if (token[0].ToUpper() == "HLT") {
                    assembled.Add(0b00011111);
                }
                //if the operand is invalid throw an error
                else {
                    throw new UnknownOperandException("Unknown Operand " + token[0] + " at address line " + i);
                }
            }
            if (verbose) Console.WriteLine("code assembled");
        }

        //writes assembled list to  a binary file with the same root name as the text file
        static void writeBinFile(string path) {
            using (BinaryWriter binWriter = new BinaryWriter(File.Open(path, FileMode.Create))) {
                foreach (byte RAMLine in assembled) {
                    binWriter.Write(RAMLine);
                }
            }
            if (verbose) Console.WriteLine("binary written to " + path);
        }
        //writes binary file over the serial port (obsolete) 
        static void binToSerial(string path) {

            byte[] serialOut;
            byte binLength;

            if (File.Exists(path)) {
                using (BinaryReader reader = new BinaryReader(File.Open(path, FileMode.Open))) {
                    binLength = (byte)(reader.BaseStream.Length);
                    serialOut = new byte[binLength * 2 + 1];

                    serialOut[0] = (byte)(binLength);

                    for (int i = 0; i < binLength; i++) {
                        serialOut.SetValue((byte)i, (byte)(i * 2 + 1));
                        serialOut.SetValue(reader.ReadByte(), (byte)(i * 2 + 2));
                    }

                    SerialPort serialPort = new SerialPort();

                    // Allow the user to set the appropriate properties.
                    serialPort.PortName = port;
                    serialPort.BaudRate = 9600;
                    serialPort.Parity = Parity.None;
                    serialPort.DataBits = 8;
                    serialPort.StopBits = StopBits.One;
                    serialPort.Handshake = Handshake.None;
                    // Set the read/write timeouts
                    serialPort.ReadTimeout = 5000;
                    serialPort.WriteTimeout = 500;

                    serialPort.Open();
                    //byte[] buffer = new byte[7];
                    //Console.WriteLine(serialPort.Read(buffer, 0, 7));
                    //string converted = Encoding.ASCII.GetString(serialOut, 0, serialOut.Length);
                    //Console.WriteLine(converted);
                    serialPort.Write(serialOut, 0, serialOut.Length);
                    serialPort.Close();
                }
            }
            else {
                Console.WriteLine("Path: " + path + " dosen't exist");
            }



        }

        //writes assembled list to the serial port
        static void serialOut() {

            byte[] serialOut;
            byte binLength;

            //microcontroler takes a byte array in the form {program length in bytes,address,data,address,data...}
            //this is building the serial out array in the correct format from the assembled list
            binLength = (byte)(assembled.Count);
            serialOut = new byte[binLength * 2 + 1];
            serialOut[0] = (byte)(binLength);
            for (int i = 0; i < binLength; i++) {
                serialOut.SetValue((byte)i, (byte)(i * 2 + 1));
                serialOut.SetValue(assembled[i], (byte)(i * 2 + 2));
            }
            //open and configure a serial port
            SerialPort serialPort = new SerialPort();
            serialPort.PortName = port;
            serialPort.BaudRate = 9600;
            serialPort.Parity = Parity.None;
            serialPort.DataBits = 8;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.None;
            serialPort.ReadTimeout = 5000;
            serialPort.WriteTimeout = 500;

            serialPort.Open();
            if (verbose) Console.WriteLine("Port opened on " + port);

            //write byte array to serial port and close 
            serialPort.Write(serialOut, 0, serialOut.Length);
            if (verbose) Console.WriteLine("Data written to " + port);
            serialPort.Close();
            if (verbose) Console.WriteLine("Port closed on " + port);
        }

        //writes assembled list to the TCP socket
        static void wifiOut() {
            byte[] wifiOut;
            byte binLength;

            //microcontroler takes a byte array in the form {program length in bytes,address,data,address,data...}
            //this is building the wifi out array in the correct format from the assembled list
            binLength = (byte)(assembled.Count);
            wifiOut = new byte[binLength * 2 + 1];
            wifiOut[0] = (byte)(binLength);
            for (int i = 0; i < binLength; i++) {
                wifiOut.SetValue((byte)i, (byte)(i * 2 + 1));
                wifiOut.SetValue(assembled[i], (byte)(i * 2 + 2));
            }

            try {

                //set ip address, port, and create a TCP socket
                IPAddress ipAddress = IPAddress.Parse("192.168.4.1");
                IPEndPoint remoteEP = new IPEndPoint(ipAddress, int.Parse(port));
                Socket sender = new Socket(ipAddress.AddressFamily,
                    SocketType.Stream, ProtocolType.Tcp);

                //try to connect the socket to the microcontroller. Catch any errors.  
                try {
                    sender.Connect(remoteEP);

                    if(verbose) Console.WriteLine("Socket connected to {0}", sender.RemoteEndPoint.ToString());

                    // Send byte array through the socket
                    int bytesSent = sender.Send(wifiOut);
                    if (verbose) Console.WriteLine("Data written to {0}", sender.RemoteEndPoint.ToString());
                    // close the socket.  
                    sender.Shutdown(SocketShutdown.Both);
                    sender.Close();
                    if (verbose) Console.WriteLine("Socket closed {0}:{1}", ipAddress.ToString(), port.ToString());
                }
                //catch errors
                catch (ArgumentNullException ane) {
                    Console.WriteLine("ArgumentNullException : {0}", ane.ToString());
                    Console.Read();
                }
                catch (SocketException se) {
                    Console.WriteLine("SocketException : {0}", se.ToString());
                    Console.Read();
                }
                catch (Exception e) {
                    Console.WriteLine("Unexpected exception : {0}", e.ToString());
                    Console.Read();
                }

            }
            catch (Exception e) {
                Console.WriteLine(e.ToString());
                Console.Read();
            }
        }
    }
    //custom exceptions
    public class UnknownLabelException : Exception {
        public UnknownLabelException(string message) : base(message) {
        }
    }
    public class UnknownOperandException : Exception {
        public UnknownOperandException(string message) : base(message) {
        }
    }
}
