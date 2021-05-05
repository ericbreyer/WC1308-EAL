using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Text;

namespace WC1308_EAB_Assembler1 {
    class Program {
        static void Main(string[] args) {

            string readFilePath;

            if (Array.Exists(args, element => element.ToUpper() == "--HELP")) {
                Console.WriteLine("USEAGE");
                Console.WriteLine("Required Params");
                Console.WriteLine("--COM [com] (COM port)");
                Console.WriteLine("--p [path] (program file path)");
                Console.WriteLine("Optional Params");
                Console.WriteLine("--help (shows this menu)");
                Console.WriteLine("--bin (output to binary file)");
                Console.WriteLine("--v (verbose)");
                return;
            }
            if (Array.Exists(args, element => element.ToUpper() == "--V")) {
                verbose = true;
                Console.WriteLine("verbose output\n");
            }

            if (Array.Exists(args, element => element.ToUpper() == "--COM")) {
                COMPort = args[Array.FindIndex(args, element => element.ToUpper() == "--COM") + 1];
            }
            else {
                Console.WriteLine("ERROR: No COM port specified");
                return;
            }
            if (Array.Exists(args, element => element.ToUpper() == "--P")) {
                readFilePath = args[Array.FindIndex(args, element => element.ToUpper() == "--P") + 1];
            }
            else {
                Console.WriteLine("ERROR: No file path specified");
                return;
            }

            try {
                primeForAssembly(readFilePath);
                parseLabels();
                assemble();
                serialOut();
                if (Array.Exists(args, element => element.ToUpper() == "--BIN")) {
                    string writeFilePath = readFilePath.Remove(readFilePath.Length - 3, 3) + "bin";
                    writeBinFile(writeFilePath);

                }
            }
            catch (FileNotFoundException e) {
                Console.WriteLine("ERROR: " + e.Message);
            }
            catch (IOException e) {
                Console.WriteLine("ERROR: " + e.Message);
                Console.WriteLine("Available Ports:");
                foreach (string s in SerialPort.GetPortNames()) {
                    Console.WriteLine("   {0}", s);
                }
            }
            catch (Exception e) {
//                Console.WriteLine("Unhandled Exception");
                Console.WriteLine(e.Message);
            }
        }


        static Dictionary<string, byte> Labels = new Dictionary<string, byte>();

        static List<string> lines = new List<string>();
        static byte linesInFile = 0;
        static List<byte> assembled = new List<byte>();
        static string COMPort;
        static bool verbose = false;
        static void primeForAssembly(string filePath) {

            string line;

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

        static void parseLabels() {

            byte nextAddress = 0;

            for (int i = 0; i < linesInFile; ++i) {
                string[] token = lines[i].Split(new char[] { ' ', ','});
                if (lines[i][0] == '#' || lines[i][0] == ';') {
                    continue;
                }
                else if (lines[i][0] == '.') {
                    string label = lines[i].Replace(".", String.Empty);
                    Labels.Add(label, (byte)(nextAddress));
                }
                else if (token[0] == "NOP" || token[0] == "OUT" || token[0] == "ADR" || token[0] == "SUR" || token[0] == "INC" || token[0] == "DEC" || token[0] == "HLT") {
                    ++nextAddress;
                }
                else {
                    nextAddress += 2;
                }
            }
            if (verbose) Console.WriteLine("labels parsed");
        }


        static void assemble() {

            byte _base = 2;

            for (int i = 0; i < linesInFile; ++i) {
                string[] token = lines[i].Split(new char[] { ' ', ',' });

                if (lines[i][0] == '.' || lines[i][0] == '#') {
                    continue;
                }
                if(i==0 && lines[i][0] == ';' && token[0] == ";base") {
                    _base = Convert.ToByte(token[1], 10);
                    if (verbose) Console.WriteLine("base encoding mode: " + _base);
                    continue;
                }
                else if (lines[i][0] == ';')  {
                    Console.WriteLine("WARNING: invalid metadata line " + i);
                    continue;
                }

                if (token[0].ToUpper() == "NOP") {
                    assembled.Add(0b00000000);
                }
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
                    assembled.Add(0b00001010);
                }
                else if (token[0].ToUpper() == "DEC") {
                    assembled.Add(0b00001100);
                }
                else if (token[0].ToUpper() == "STA") {
                    assembled.Add(0b00001101);
                    byte data = Convert.ToByte(token[1], _base);
                    assembled.Add(data);
                }
                else if (token[0].ToUpper() == "JMP") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00001110);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        Console.WriteLine("WARNING: Unknown Label " + token[1] + " line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JC") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00001111);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        Console.WriteLine("WARNING: Unknown Label " + token[1] + " line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JZ") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010000);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        Console.WriteLine("WARNING: Unknown Label " + token[1] + " line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JNC") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010001);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        Console.WriteLine("WARNING: Unknown Label " + token[1] + " line " + i);
                    }
                }
                else if (token[0].ToUpper() == "JNZ") {
                    if (Labels.ContainsKey(token[1])) {
                        assembled.Add(0b00010010);
                        assembled.Add(Labels[token[1]]);
                    }
                    else {
                        Console.WriteLine("WARNING: Unknown Label " + token[1] + " line " + i);
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
                else {
                    Console.WriteLine("WARNING: Unknown operand " + token[1] + " line " + i);
                }
            }
            if (verbose) Console.WriteLine("code assembled");
        }


        static void writeBinFile(string path) {
            using (BinaryWriter binWriter = new BinaryWriter(File.Open(path, FileMode.Create))) {
                foreach (byte RAMLine in assembled) {
                    binWriter.Write(RAMLine);
                }
            }
            if (verbose) Console.WriteLine("binary written to " + path);
        }

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
                    serialPort.PortName = COMPort;
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


        static void serialOut() {

            byte[] serialOut;
            byte binLength;
            binLength = (byte)(assembled.Count);
            serialOut = new byte[binLength * 2 + 1];
            serialOut[0] = (byte)(binLength);

            for (int i = 0; i < binLength; i++) {
                serialOut.SetValue((byte)i, (byte)(i * 2 + 1));
                serialOut.SetValue(assembled[i], (byte)(i * 2 + 2));
            }

            SerialPort serialPort = new SerialPort();
            if (verbose) Console.WriteLine("Port opened on " + COMPort);
            // Allow the user to set the appropriate properties.
            serialPort.PortName = COMPort;
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
            if (verbose) Console.WriteLine("Data written to " + COMPort);
            serialPort.Close();
            if (verbose) Console.WriteLine("Port closed on " + COMPort);
        }
    }
}
