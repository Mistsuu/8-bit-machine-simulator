#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>

#if defined(WIN32) && !defined(__unix__)
  #include <windows.h>
  #include <conio.h>
#elif defined(__unix__) && !defined(WIN32)
  #include <ncurses.h>
  #include <unistd.h>
  #include <signal.h>
#else

#endif

using namespace std;

#define CLK_SPEED 100 // Limited to 100 HZ

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
string  Argument;        /* for debugging only, not in actual machine */
static volatile int ProgramRun = 1;

////////////////////// Output modes //////////////////////////////////////
const uint8_t SIGNED   = 0;
const uint8_t UNSIGNED = 1;
const uint8_t MANUAL   = 0;
const uint8_t AUTO     = 1;
const uint8_t ENTER    = 10;
const uint8_t SPACE    = 32;

////////////////////// Program infos ///////////////////////////////////////

uint8_t RAMContent[256];     // To simulate memory of 256 bytes of codes
int cycleCounting = 0;

////////////////////// Screen handling ///////////////////////////////
// To let console know if we need to wipe the screen
#if defined(WIN32) && !defined(__unix__)
  uint8_t OutputMode    = SIGNED;
  uint8_t DebugMode     = MANUAL;
  bool    startDisplay  = true;  
  COORD   cursorPos;

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
    writeBlank();
  //  resetCursor();
  }

  bool getStartLocation()
  {
    #ifdef(_WIN32)
      CONSOLE_SCREEN_BUFFER_INFO csbi;
      if (!GetConsoleScreenBufferInfo(
            GetStdHandle( STD_OUTPUT_HANDLE ),
            &csbi
          ))
        return false;

      cursorPos = csbi.dwCursorPosition;
      return true;
    #endif
  }

  void outputBinary(uint8_t number) {
    for (int i = 7; i >= 0; --i) {
      cout << (((number >> i) & 0x1) ? "1" : "0");
    }
  }

  void outputHex(uint8_t number) {
    char hexAlphabet[] = "0123456789abcdef";
    cout << hexAlphabet[number >> 4] << hexAlphabet[number & 0xf];
  }

  inline void displayInfo() {
    cout << "[] Memory:\n";
    for (int i = 0; i < 16; ++i) {
      cout << "   ";
      for (int j = 0; j < 16; ++j) {
        safe_printw("%02x ", RAMContent[i*16+j]);
        if (j == 7)
          cout << " ";
      }
      cout << endl;
    }
    cout << endl;

    cout << "[] Mem Register    : "; outputBinary(MemRegister);    cout << "   " << "[] Ram Content  : "; outputBinary(RAMContent[MemRegister]); cout << endl;
    cout << "[] A   Register    : "; outputBinary(ARegister);      cout << "   " << "[] B   Register : "; outputBinary(BRegister);               cout << endl;
    cout << "[] Sum Register    : "; outputBinary(SumRegister);    cout << "   " << "(ZF: " << unsigned(ZeroFlag) << ", CF: " << unsigned(CarryFlag) << ")" << endl;
    cout << "[] Program Counter : "; outputBinary(ProgramCounter); cout << "   " << "[] Instruction  : "; outputBinary(Instruction);
    
    cout << " -> ";
    switch(Instruction) {
      case LDA:
        cout << "LDA " << Argument;
        break;
      case ADD:
        cout << "ADD " << Argument;
        break;
      case SUB:
        cout << "SUB " << Argument;
        break;
      case STA:
        cout << "STA " << Argument;
        break;
      case LDI:
        cout << "LDI " << Argument;
        break;
      case JMP:
        cout << "JMP " << Argument;
        break;
      case JC:
        cout << "JC " << Argument;
        break;
      case JZ:
        cout << "JZ " << Argument;
        break;
      case AEI:
        cout << "AEI " << Argument;
        break;
      case SEI:
        cout << "SEI " << Argument;
        break;
      case SHL:
        cout << "SHL " << Argument;
        break;
      case HLT:
        cout << "HLT " << Argument;
        break;
      case _OUT:
        cout << "OUT " << Argument;
        break;
      case SLF:
        cout << "SLF " << Argument;
        break;
      case NOP:
        cout << "NOP " << Argument;
        break;
      default:
        cout << "Not recognized";
        ProgramRun = 0;
        break;
    }
    cout << endl;

    cout << ">>> Output: [[";
    if (OutputMode == SIGNED)
      cout << signed(OutRegister);
    else
      cout << unsigned(OutRegister);
    cout << "]]  ";
  }

  void printInstruction() {
    cout << "========================================================"                << endl;
    cout << "    Press SPACE to single step the code."                                << endl;
    cout << "    Press ENTER to automatically run the code. (" << CLK_SPEED << "Hz)." << endl;
    cout << "    Press CTRL-C to exit the program."                                   << endl;
    cout << "    (NOTE: if you press ENTER there's no going back.)"                   << endl;
    cout << "========================================================"                << endl;
  }

  bool controlDisplay() {
    if (DebugMode == MANUAL) {
      FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
      while (!_kbhit())
        if (ProgramRun == 0) 
          return false;

      char ch = _getch();
      if (ch == ' ')
        return true;
      else if (ch == ENTER)
        DebugMode = AUTO;
      else
        return controlDisplay();
    }

    if (DebugMode == AUTO) {
      Sleep(1000 / CLK_SPEED);
    }
    
    return true;
  }

  bool updateDisplay() {
    if (startDisplay) {
      printInstruction();
      getStartLocation();
      startDisplay = false;
    }
    else {
      clearOutput();
    }

    displayInfo();
    return controlDisplay();
  }
  
#elif defined(__unix__) && !defined(WIN32)
  uint8_t OutputMode   = SIGNED;
  uint8_t DebugMode    = MANUAL;
  bool    startDisplay = true;

  #define safe_printw(...)      \
  do {                        \
    if (ProgramRun)           \
      printw(__VA_ARGS__);    \
  } while(0)                

  void clearOutput() {
    if (ProgramRun) {
      move(7, 0);
      clrtoeol();
    }
  }

  int kbhit() {
    int ch = getch();
    if (ch != ERR) {
        ungetch(ch);
        return 1;
    } 
    return 0;
  }

  void printInstruction() {
    safe_printw("==============================================================\n");
    safe_printw("    Press SPACE to single step the code.                      \n");
    safe_printw("    Press ENTER to automatically run the code (%d Hz).        \n", CLK_SPEED);
    safe_printw("    Press CTRL-C to exit the program.                         \n");
    safe_printw("    (NOTE: if you press ENTER there's no going back.)         \n");
    safe_printw("==============================================================\n");
    safe_printw("\n");
  }

  bool controlDisplay() {
    if (DebugMode == MANUAL) {
      while (!kbhit())
        if (ProgramRun == 0) 
          return false;

      char ch = getch();
      if (ch == ' ')
        return true;
      else if (ch == ENTER)
        DebugMode = AUTO;
      else
        return controlDisplay();
    }

    if (DebugMode == AUTO) {
      refresh();
      fflush(stdout);
      usleep(1000000 / CLK_SPEED);
    }
    
    return true;
  }

  void outputBinary(uint8_t number) {
    for (int i = 7; i >= 0; --i)
      safe_printw(((number >> i) & 0x1) ? "1" : "0");
  }

  inline void displayInfo() {
    safe_printw("[] Memory:\n");
    for (int i = 0; i < 16; ++i) {
      safe_printw("   ");
      for (int j = 0; j < 16; ++j) {
        safe_printw("%02x ", RAMContent[i*16+j]);
        if (j == 7)
          safe_printw(" ");
      }
      safe_printw("\n");
    }
    safe_printw("\n");

    safe_printw("[] Mem Register    : "); outputBinary(MemRegister);    safe_printw("   "); safe_printw("[] Ram Content  : "); outputBinary(RAMContent[MemRegister]);                               safe_printw("\n");
    safe_printw("[] A   Register    : "); outputBinary(ARegister);      safe_printw("   "); safe_printw("[] B   Register : "); outputBinary(BRegister);                                             safe_printw("\n");
    safe_printw("[] Sum Register    : "); outputBinary(SumRegister);    safe_printw("   "); safe_printw("(ZF: "); safe_printw("%u", ZeroFlag); safe_printw(", CF: "); safe_printw("%u", CarryFlag); safe_printw(")"); safe_printw("\n");
    safe_printw("[] Program Counter : "); outputBinary(ProgramCounter); safe_printw("   "); safe_printw("[] Instruction  : "); outputBinary(Instruction); 
    
    safe_printw(" -> ");
    switch(Instruction) {
      case LDA:
        safe_printw("LDA %s", &Argument[0]);
        break;
      case ADD:
        safe_printw("ADD %s", &Argument[0]);
        break;
      case SUB:
        safe_printw("SUB %s", &Argument[0]);
        break;
      case STA:
        safe_printw("STA %s", &Argument[0]);
        break;
      case LDI:
        safe_printw("LDI %s", &Argument[0]);
        break;
      case JMP:
        safe_printw("JMP %s", &Argument[0]);
        break;
      case JC:
        safe_printw("JC %s", &Argument[0]);
        break;
      case JZ:
        safe_printw("JZ %s", &Argument[0]);
        break;
      case AEI:
        safe_printw("AEI %s", &Argument[0]);
        break;
      case SEI:
        safe_printw("SEI %s", &Argument[0]);
        break;
      case SHL:
        safe_printw("SHL %s", &Argument[0]);
        break;
      case HLT:
        safe_printw("HLT %s", &Argument[0]);
        break;
      case _OUT:
        safe_printw("OUT %s", &Argument[0]);
        break;
      case SLF:
        safe_printw("SLF %s", &Argument[0]);
        break;
      case NOP:
        safe_printw("NOP %s", &Argument[0]);
        break;
      default:
        safe_printw("Not recognized");
        ProgramRun = 0;
        break;
    }
    safe_printw("\n");

    safe_printw("\n");
    safe_printw(">>> Output: [[");
    if (OutputMode == SIGNED)
      safe_printw("%d", OutRegister);
    else
      safe_printw("%u", OutRegister);
    safe_printw("]]  \n");
  }

  bool updateDisplay() {
    if (startDisplay) {
      printInstruction();
      // getStartLocation();
      startDisplay = false;
    }
    else {
      clearOutput();
    }

    displayInfo();
    return controlDisplay();
  }
#else
#endif

////////////////////// Main loop ///////////////////////////////////

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

bool updateMachine() {
  cycleCounting++;
  return updateDisplay();
}

void run() {
  initRegisters();

  while (ProgramRun) {
    if (!updateMachine())
      return;

    // Fetch Instruction
    MemRegister = ProgramCounter++;
    Instruction = RAMContent[MemRegister];
    Argument    = "";
    if (!updateMachine())
      return;

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
        Argument    = to_string(RAMContent[MemRegister]);
        if (!updateMachine())
          return;
        break;
    }

    // Handling instructions
    switch(Instruction) {
      case LDA:
        MemRegister = RAMContent[MemRegister];
        if (!updateMachine())
          return;

        ARegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case ADD:
        MemRegister = RAMContent[MemRegister];
        if (!updateMachine())
          return;

        BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateMachine())
          return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case SUB:
        MemRegister = RAMContent[MemRegister];
        if (!updateMachine())
          return;

        BRegister = RAMContent[MemRegister];
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, true, true);
      	if (!updateMachine())
          return;

      	ARegister = SumRegister;
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
      	break;

      case STA:
        MemRegister = RAMContent[MemRegister];
        if (!updateMachine())
          return;

      	RAMContent[MemRegister] = ARegister;
      	if (!updateMachine())
          return;
      	break;

      case LDI:
        ARegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case JC:
        if (CarryFlag == 0) 
          break;

        ProgramCounter = RAMContent[MemRegister];
        if (!updateMachine())
          return;
        break;

      case JZ:
        if (ZeroFlag == 0) 
          break;

        ProgramCounter = RAMContent[MemRegister];
        if (!updateMachine())
          return;
        break;

      case JMP:
        ProgramCounter = RAMContent[MemRegister];
        if (!updateMachine())
          return;
        break;

      case AEI:
        BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateMachine())
          return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case SEI:
        BRegister = RAMContent[MemRegister];
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, true, true);
      	if (!updateMachine())
          return;

      	ARegister = SumRegister;
      	SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
      	break;

      case SHL:
        MemRegister = RAMContent[MemRegister];
        if (!updateMachine())
          return;

        ARegister = BRegister = RAMContent[MemRegister];
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateMachine())
          return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case HLT:
        ProgramRun = 0;
        break;

      case _OUT:
        OutRegister = ARegister;
        if (!updateMachine())
          return;
        break;

      case SLF:
        BRegister = ARegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, true);
        if (!updateMachine())
          return;

        ARegister = SumRegister;
        SumRegister = performArithmetic(ARegister, BRegister, ZeroFlag, CarryFlag, false, false);
        if (!updateMachine())
          return;
        break;

      case NOP:
      default:
        break;
    }
  }
}

////////////////////// Initialize /////////////////////////////
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
  if (byteLine.length() != 19) 
    return false;

  // 8 first & last characters are binary code, seperated by a " | "
  for (unsigned int i = 0; i < byteLine.length(); ++i) {
    if ((i <= 7 || i >= 11) && (byteLine[i] != '1' && byteLine[i] != '0')) return false;
    if ((i == 8 || i == 10) && (byteLine[i] != ' '))                       return false;
    if ((i == 9)            && (byteLine[i] != '|'))                       return false;
  }

  byteLine.erase(0, 11);
  return true;
}

// A function to keep the rule of OPCodes
uint8_t extractOPCode(string byteLine) {
  uint8_t opcode = 0;
  for (int i = 0; i < 4; ++i)
    if (byteLine[i] == '1') 
      opcode |= (1 << (3 - i));
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

bool initScreen() {
  initscr();

  cbreak();
  noecho();
  nodelay(stdscr, TRUE);

  scrollok(stdscr, TRUE);
  return true;
}

////////////////////// Main ///////////////////////////////////

void checkInterupt(int signal) {
  // Stops the program.
  ProgramRun = 0;
  cout << endl;
  cout << "[debug] Program abruptly exit after " << cycleCounting << " cycles." << endl;
}

void closeProgram() {
  #if defined(WIN32) && !defined(__unix__)
    cout << endl;
    cout << "[debug] Program finished after " << cycleCounting << " cycles." << endl;
  #elif defined(__unix__) && !defined(WIN32)
    printw("\n");
    printw("Program finished after %d cycles.\n", cycleCounting);
    printw("Press anykey to quit...\n");
    while (!kbhit()) 
      usleep(100000);
    endwin();
  #endif
}

int main(int argc, char* argv[]) {
  if (!checkArgumentError(argc, argv)) 
    return -1;
  
  if (!checkData(string(argv[1])))
    return -2;

  if (!initScreen())
    return -3;
  
  #if defined(WIN32) && !defined(__unix__)
    signal(SIGINT, checkInterupt);
  #endif

  run();
  closeProgram();
}
