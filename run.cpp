#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <windows.h>
#include <conio.h>
using namespace std;

#define CLK_SPEED 1000 // Limited to 100 HZ

#define NOP 0b0000
#define LDA 0b0001
#define ADD 0b0010
#define SUB 0b0011
#define STA 0b0100
#define LDI 0b0101
#define JMP 0b0110
#define JC  0b0111
#define JZ  0b1000
#define AEI 0b1001
#define SEI 0b1010
#define SHL 0b1011
#define SLF 0b1101
#define _OUT 0b1110
#define HLT 0b1111

//////////////////////// For Program ///////////////////////////////////
uint8_t MemRegister;
uint8_t ARegister;
uint8_t BRegister;
uint8_t SumRegister;
uint8_t Instruction;
uint8_t ProgramCounter;
uint8_t OutRegister;
uint8_t ZeroFlag;
uint8_t CarryFlag;
static volatile int ProgramRun = 1;

////////////////////// Output modes //////////////////////////////////////
const uint8_t SIGNED   = 0;
const uint8_t UNSIGNED = 1;
const uint8_t MANUAL   = 0;
const uint8_t AUTO     = 1;
const uint8_t ENTER    = 13;
const uint8_t SPACE    = 32;

////////////////////// Output info ///////////////////////////////////////
COORD cursorPos;

uint8_t RAMContent[256];     // To simulate memory of 256 bytes of codes
bool startDisplay = true;    // To let console know if we need to wipe the screen
uint8_t OutputMode = SIGNED;
uint8_t DebugMode  = MANUAL;

int cycleCounting = 0;

void outputBinary(uint8_t number) {
  for (int i = 7; i >= 0; --i) {
    cout << (((number >> i) & 0x1) ? "1" : "0");
  }
}

bool checkArgumentError(int argc, char* argv[]) {
  if (argc != 2) {
    cout << "[error] Exactly one argument required." << endl;
    return false;
  }

  if (string(argv[1]).find(".out") == string::npos) {
    cout << "[error] Wrong input format filename. Filename \"" << string(argv[1]) << "\" does not start with \".out\"!" << endl;
    return false;
  }

  return true;
}

// A function to keep the rule of byte code file
bool checkByteLine(string &byteLine) {
  // A line should have the length of 19
  if (byteLine.length() != 19) return false;

  // 8 first & last characters are binary code, seperated by a " | "
  for (unsigned int i = 0; i < byteLine.length(); ++i) {
    if ((i <= 7 || i >= 11) && (byteLine[i] != '1' && byteLine[i] != '0')) return false;
    if ((i == 8 || i == 10) && (byteLine[i] != ' '))                       return false;
    if ((i == 9)            && (byteLine[i] != '|'))                       return false;
  }

  byteLine.erase(0, 11);
  return true;
}

// A function to keep the rule of op code
uint8_t extractOPCode(string byteLine) {
  uint8_t opcode = 0;
  for (int i = 0; i < 4; ++i) {
    if (byteLine[i] == '1') opcode |= (1 << (3 - i));
  }
  return opcode;
}

uint8_t extractArgument(string byteLine) {
  uint8_t argumentData = 0;
  for (int i = 0; i < 8; ++i) {
    if (byteLine[i] == '1') argumentData |= (1 << (7 - i));
  }
  return argumentData;
}

bool checkData(string filename) {
  cout << "[debug] Checking validity of file..." << endl;

  fstream byteFile;             // open machine code file
  string  byteLine;             // line of byte code
  string  getMode = "command";  // mode of getting file

  byteFile.open(filename, fstream::in);
  if (!byteFile) {
    cout << "[error] Cannot open code file \"" << filename << "\"!" << endl;
    return false;
  }

  for (int i = 0; i < 256; ++i) {
    if (byteFile.eof()) {
      cout << "[error] Insufficient amount of byte code!" << endl;
      return false;
    }

    getline(byteFile, byteLine);
    if (!checkByteLine(byteLine)) {
      cout << "[error] Wrong format for the file!" << endl;
      return false;
    }

    // Get data in accordance to mode
    if (getMode == "command") {
      RAMContent[i] = extractOPCode(byteLine);
      switch(RAMContent[i]) {
        case LDA:
        case ADD:
        case SUB:
        case STA:
        case LDI:
        case JMP:
        case JC:
        case JZ:
        case AEI:
        case SEI:
        case SHL:
          getMode = "argument";
          break;
      }
    }
    else if (getMode == "argument") {
      RAMContent[i] = extractArgument(byteLine);
      getMode = "command";
    }
  }

  cout << "[debug] File ready to run." << endl;
  return true;
}

inline void resetCursor() {
  SetConsoleCursorPosition(
    GetStdHandle(STD_OUTPUT_HANDLE),
    cursorPos
    );
}

inline bool writeBlank() {
  // Get screen size
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  COORD screenSize;
  if (!GetConsoleScreenBufferInfo(
        GetStdHandle( STD_OUTPUT_HANDLE ),
        &csbi
        ))
    return false;
  screenSize = csbi.dwSize;

  // Write ' ' to the next 5 lines
  string screenBuf = "";
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < screenSize.X; ++x) {
      screenBuf += " ";
    }
    screenBuf += "\n";
  }
  cout << screenBuf;
  return true;
}

void clearOutput() {
  resetCursor();
//  writeBlank();
//  resetCursor();
}

bool getStartLocation()
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(
        GetStdHandle( STD_OUTPUT_HANDLE ),
        &csbi
        ))
    return false;

  cursorPos = csbi.dwCursorPosition;
  return true;
}

inline void displayInfo() {
  cout << "[] Mem Register   : "; outputBinary(MemRegister);    cout << "   " << "[] Ram Content : "; outputBinary(RAMContent[MemRegister]); cout << endl;
  cout << "[] A   Register   : "; outputBinary(ARegister);      cout << "   " << "[] B   Register: "; outputBinary(BRegister);               cout << endl;
  cout << "[] Sum Register   : "; outputBinary(SumRegister);    cout << "   " << "(ZF: " << unsigned(ZeroFlag) << ", CF: " << unsigned(CarryFlag) << ")" << endl;
  cout << "[] Program Counter: "; outputBinary(ProgramCounter); cout << "   " << "[] Instruction : "; outputBinary(Instruction); cout << "(";
  switch(Instruction) {
    case LDA:
      cout << "LDA";
      break;
    case ADD:
      cout << "ADD";
      break;
    case SUB:
      cout << "SUB";
      break;
    case STA:
      cout << "STA";
      break;
    case LDI:
      cout << "LDI";
      break;
    case JMP:
      cout << "JMP";
      break;
    case JC:
      cout << "JC";
      break;
    case JZ:
      cout << "JZ";
      break;
    case AEI:
      cout << "AEI";
      break;
    case SEI:
      cout << "SEI";
      break;
    case SHL:
      cout << "SHL";
      break;
    case HLT:
      cout << "HLT";
      break;
    case _OUT:
      cout << "OUT";
      break;
    case SLF:
      cout << "SLF";
      break;
    case NOP:
      cout << "NOP";
      break;
    default:
      cout << "Not recognized";
      ProgramRun = 0;
      break;
  }
  cout << ") " << endl;
  cout << ">>> Output: [[";
  if (OutputMode == SIGNED)
     cout << signed(OutRegister);
  else
     cout << unsigned(OutRegister);
  cout << "]]  ";
}

bool controlDisplay() {
  if (DebugMode == MANUAL) {
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    while (!_kbhit()) {
      if (ProgramRun == 0) return false;
    }

    char ch = _getch();
    if (ch == ' ')
      return true;
    else if (ch == ENTER) {
      DebugMode = AUTO;
    }
    else {
      return controlDisplay();
    }
  }

  if (DebugMode == AUTO) {
    Sleep(1000 / CLK_SPEED);
  }
  
  return true;
}

bool updateDisplay() {
  cycleCounting++;
  if (startDisplay) {
    getStartLocation();
    startDisplay = false;
  }
  else {
    clearOutput();
  }

  displayInfo();
  if (!controlDisplay()) return false;

  return true;
}

uint8_t performArithmetic(uint8_t A, uint8_t B, uint8_t &ZeroFlag, uint8_t &CarryFlag, bool SUB_FLAG, bool FLAG_IN) {
  uint16_t A_16   = A & 0b11111111;
  uint16_t B_16   = B & 0b11111111;
  uint16_t result_16;
  if (SUB_FLAG)
    result_16 = A_16 + (~B_16 & 0b11111111) + 1;
  else
    result_16 = A_16 + B_16;

  if (FLAG_IN) {
    CarryFlag = (result_16 >> 8) & 0x1;
    ZeroFlag  = ((result_16 & 0b11111111) == 0);
  }

  uint8_t result_8 = result_16 & 0b11111111;
  return result_8;
}

void initRegisters() {
  MemRegister    = 0;
  ARegister      = 0;
  BRegister      = 0;
  SumRegister    = 0;
  Instruction    = 0;
  ProgramCounter = 0;
  OutRegister    = 0;
  ZeroFlag       = 0;
  CarryFlag      = 0;
  ProgramRun     = 1;
}

void run() {
  cout << "[debug] Running the program..." << endl;
  initRegisters();

  while (ProgramRun) {
    if (!updateDisplay()) return;

    // Fetch Instruction
    MemRegister = ProgramCounter++;
    Instruction = RAMContent[MemRegister];
    if (!updateDisplay()) return;

    // Get argument
    switch(Instruction) {
      case LDA:
      case ADD:
      case SUB:
      case STA:
      case LDI:
      case JMP:
      case JC:
      case JZ:
      case AEI:
      case SEI:
      case SHL:
        MemRegister = ProgramCounter++;
        if (!updateDisplay()) return;
        break;
    }

    // Handling instructions
    switch(Instruction) {
      case LDA:
        MemRegister = RAMContent[MemRegister];
        if (!updateDisplay()) return;

        ARegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case ADD:
        MemRegister = RAMContent[MemRegister];
        if (!updateDisplay()) return;

        BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateDisplay()) return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case SUB:
        MemRegister = RAMContent[MemRegister];
        if (!updateDisplay()) return;

        BRegister = RAMContent[MemRegister];
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, true, true);
      	if (!updateDisplay()) return;

      	ARegister = SumRegister;
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
      	break;

      case STA:
        MemRegister = RAMContent[MemRegister];
        if (!updateDisplay()) return;

      	RAMContent[MemRegister] = ARegister;
      	if (!updateDisplay()) return;
      	break;

      case LDI:
        ARegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case JC:
        if (CarryFlag == 0) break;

        ProgramCounter = RAMContent[MemRegister];
        if (!updateDisplay()) return;
        break;

      case JZ:
        if (ZeroFlag == 0) break;

        ProgramCounter = RAMContent[MemRegister];
        if (!updateDisplay()) return;
        break;

      case JMP:
        ProgramCounter = RAMContent[MemRegister];
        if (!updateDisplay()) return;
        break;

      case AEI:
        BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateDisplay()) return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case SEI:
        BRegister = RAMContent[MemRegister];
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, true, true);
      	if (!updateDisplay()) return;

      	ARegister = SumRegister;
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
      	break;

      case SHL:
        MemRegister = RAMContent[MemRegister];
        if (!updateDisplay()) return;

        ARegister = BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateDisplay()) return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case HLT:
        ProgramRun = 0;
        break;

      case _OUT:
        OutRegister = ARegister;
        if (!updateDisplay()) return;
        break;

      case SLF:
        BRegister = ARegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateDisplay()) return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateDisplay()) return;
        break;

      case NOP:
      default:
        break;
    }
  }
}

void checkInterupt(int signal) {
  // Stops the program.
  ProgramRun = 0;
}

int main(int argc, char* argv[]) {
  if (!checkArgumentError(argc, argv)) return -1;
  if (checkData(string(argv[1]))) {
    signal(SIGINT, checkInterupt);
    run();
  }
  cout << endl << "[debug] Program finished after " << cycleCounting << " cycles." << endl;
}
