#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
using namespace std;

/* opcode -> bytecode */
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
#define OUT 0b1110
#define HLT 0b1111

/* alphabet */
map<string, int> code;
string           sortedVariableAlphabet   = "$0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
string           optionalOperatorAlphabet = "+-*";

//////////////////////////////////////////////////////////////////////////////////////////////
//                                   CATEGORIZE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////

inline bool isInt(string s) {
  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] < '0' || s[i] > '9') 
      return false;
  }
  return true;
}

inline bool isVariableExists(string varName, map<string, int> variableMap) {
  return !(variableMap.find(varName) == variableMap.end());
}

inline bool isInstruction(string opcode) {
  return !(code.find(opcode) == code.end());
}

inline bool isTag(string opCode) {
  return opCode[opCode.length() - 1] == ':';
}

inline bool isGoodVariableName(string varName) {
  if (isInt(varName)) {
    return false;
  }

  for (int i = 0; i < varName.length(); ++i) {
    int l = 0;
    int r = sortedVariableAlphabet.length() - 1;
    int m = -1;

    unsigned char varchr = varName[i];
    bool isInAlphabet = false;
    while (r >= l) {
      m = (l + r) / 2;
      if (varchr == (unsigned char)sortedVariableAlphabet[m]) {
        isInAlphabet = true;
        break;
      }
      else if (varchr > (unsigned char)sortedVariableAlphabet[m])
        l = m+1;
      else if (varchr < (unsigned char)sortedVariableAlphabet[m])
        r = m-1;
    }

    if (!isInAlphabet) {
      return false;
    }
  }

  return true;
}

inline bool isGoodOptionalOperator(string optionalOperator) {
  return optionalOperator.length() == 1 && optionalOperatorAlphabet.find(optionalOperator) != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//                                    CONVERT FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline string toBinaryString(T data, int bits) {
  string binData;
  for (; bits > 0; --bits) {
    binData += (((data >> (bits - 1)) & 0x1) ? "1" : "0");
  }
  return binData;
}

inline int toInteger(string numString) {
  return atoi(&numString[0]);
}

inline void filterComment(string &codeLine) {
  auto commentPos = codeLine.find('#');
  if (commentPos != string::npos) {
    codeLine.erase(commentPos);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//                                 INITIALIZE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////

void mapOPCode() {
  // For uppercase users
  code["NOP"] = NOP;
  code["LDA"] = LDA;
  code["ADD"] = ADD;
  code["SUB"] = SUB;
  code["STA"] = STA;
  code["LDI"] = LDI;
  code["JMP"] = JMP;
  code["JC" ] = JC ;
  code["JZ" ] = JZ ;
  code["AEI"] = AEI;
  code["SEI"] = SEI;
  code["SHL"] = SHL;
  code["SLF"] = SLF;
  code["OUT"] = OUT;
  code["HLT"] = HLT;
  
  // For lowercase users
  code["nop"] = NOP;
  code["lda"] = LDA;
  code["add"] = ADD;
  code["sub"] = SUB;
  code["sta"] = STA;
  code["ldi"] = LDI;
  code["jmp"] = JMP;
  code["jc" ] = JC ;
  code["jz" ] = JZ ;
  code["aei"] = AEI;
  code["sei"] = SEI;
  code["shl"] = SHL;
  code["slf"] = SLF;
  code["out"] = OUT;
  code["hlt"] = HLT;
}

void initGlobal() {
  mapOPCode();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//                                 COMPILING FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////

bool compileTags(fstream& codeFile, map<string, int>& variableMap) {
  /* Part of code */
  stringstream ssin;  // Parse int & strings
  string codeLine;    // One code line
  string opcode;      // Opcode
  string argument;    // Argument
  string tag;         // Tag

  /* Tag */
  vector<string> tagNames;        // List of tags' name
  unsigned int   tagPlace = 0;    // Position of tag in code.

  // Setting up tag
  while (getline(codeFile, codeLine)) {
    /* filter comments and 
       setup stringstream */ 
    filterComment(codeLine);     
    ssin.clear();
    ssin.str(codeLine);

    opcode = "";
    argument = "";
    ssin >> opcode;
    ssin >> argument;

    if (opcode == "") 
      continue;

    /* add tag to the list of variables */ 
    if (isTag(opcode)) { 
      tag = opcode.substr(0, opcode.length() - 1);
      if (!isGoodVariableName(tag)) {
        cout << "[error] Tag name \"" << tag << "\" is not allowed! (allowed characters: lowercase/uppercase characters, digits, _, $)" << endl;
        return false;
      }
      
      if (isVariableExists(tag, variableMap)) {
        cout << "[error] Tag \"" << tag << "\" is repeated twice in the code!" << endl;
        return false;
      }

      if (argument != "") {
        cout << "[error] There should not be anything following a tag."  << endl;
        return false;
      }

      variableMap[tag] = tagPlace;
      tagNames.push_back(tag);
      continue;
    }

    /* increment pointers to actual data */ 
    if (opcode != "") 
      tagPlace++;
    if (argument != "") 
      tagPlace++;
  }

  // Get statistic
  if (tagNames.size() > 0)
    cout << "[debug] Added following tags..." << endl;
  for (unsigned int iName = 0; iName < tagNames.size(); ++iName)
    cout << "    [+] " << tagNames[iName] << ": " << toBinaryString(variableMap[tagNames[iName]], 8) << " (" << variableMap[tagNames[iName]] << ")" << endl;
  return true;
}

bool compileInstructions(fstream& codeFile, map<string, int>& variableMap, vector<int>& InitRAMContent) {
  cout << "[debug] Compiling the instructions..." << endl;

  /* Part of code */
  stringstream ssin;          // Parse int & strings
  string codeLine;            // One code line
  string opcode;              // Opcode
  string argument;            // Argument
  string optionalOperator;    // We can add +, - , * first argument
  string optionalArgument;    // with a 2nd argument
  string tag;                 // Tag

  /* Instructions generators */
  unsigned int stackReg = 0xff;   // Store variables created in memory
  vector<string> variableNames;   // List of variables' name

  // Getting data
  while (getline(codeFile, codeLine)) {
    filterComment(codeLine);
    ssin.clear();
    ssin.str(codeLine);

    opcode = "";
    argument = "";
    optionalOperator = "";
    optionalArgument = "";
    ssin >> opcode;
    ssin >> argument;
    ssin >> optionalOperator;
    ssin >> optionalArgument;

    // Empty line.
    if (opcode == "") {
      continue;
    }

    // Print parsed assembly to STDOUT
    cout << "     -> " << opcode << " " << argument << endl;

    // Writes the raw data into memory
    // if it's integer.
    if (isInt(opcode)) {
      if (argument != "") {
        cout << "[error] For integer as raw data in the code, there can only be one number per line." << endl;
        return false;
      }

      InitRAMContent.push_back(toInteger(opcode));
      continue;
    }

    // Skip tags
    if (isTag(opcode)) {
      continue;
    }
    
    // Throw error if user
    // has bogus opcode :p
    if (!isInstruction(opcode)) {
      cout << "[error] Instruction not recognized (opcode: " << opcode << ")." << endl;
      return false;
    }

    // Shift 4 to the left as
    // I forgot to buy 1 more
    // ROM to have 8-bit
    // instruction set :'(
    InitRAMContent.push_back(code[opcode] << 4);

    int variableAddress = 0;
    int iOptionalArgument = 0;
    switch (code[opcode]) {
      /* 1 argument required (with 2 optional ones). */
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
        if (argument == "") {
          cout << "[error] Instruction \"" << opcode << "\" requires an argument." << endl;
          return false;
        }

        /*  if argument is not integer,
            meaning it could be a string variable,
            convert it to address then 
            write address to RAM. */
        if (isInt(argument)) {
          variableAddress = toInteger(argument);
        }
        else if (!isVariableExists(argument, variableMap)) {
          // check if variable name is allowed
          if (!isGoodVariableName(argument)) {
            cout << "[error] Variable name \"" << argument << "\" is not allowed! (allowed characters: lowercase/uppercase characters, digits, _, $)" << endl;
            return false;
          }

          // check if there are too many variables
          if (stackReg <= InitRAMContent.size() || stackReg == 0) {
            cout << "[error] Don't have more memory to generate more variables!" << endl;
            return false;
          }

          // generate address & map variable name to it.
          variableAddress = stackReg--;
          variableMap[argument] = variableAddress;
          variableNames.push_back(argument);
        }
        else {
          variableAddress = variableMap[argument];
        }

        /*  get optional operator & argument */
        if (optionalOperator != "") {
          if (!isGoodOptionalOperator(optionalOperator)) {
            cout << "[error] Optional operator should only be +, - or *!" << endl;
            return false;
          }

          if (optionalArgument == "") {
            cout << "[error] Optional argument required!" << endl;
            return false;
          }

          if (!isInt(optionalArgument)) {
            cout << "[error] Optional argument must be an integer!" << endl;
            return false;
          }

          iOptionalArgument = toInteger(optionalArgument);
          switch (optionalOperator[0]) {
            case '+':
              variableAddress += iOptionalArgument;
              break;
            case '-':
              variableAddress -= iOptionalArgument;
              break;
            case '*':
              variableAddress *= iOptionalArgument;
              break;
          }
        }

        // write address to RAM.
        InitRAMContent.push_back(variableAddress);
        break;

      /* 0 argument required. */
      case NOP:
      case HLT:
      case OUT:
      case SLF:
        if (argument != "") {
          cout << "[error] Argument doesn't exist for this instruction \"" << opcode << "\"." << endl;
          return false;
        }

        break;
    }
  }

  // Memory limit handling :'3
  if (InitRAMContent.size() > 256) {
    cout << "[error] Machine only has 256 addresses to store stuffs :< This code compiles to " << InitRAMContent.size() << " bytes." << endl;
    return false;
  }

  // Add the remaining memory space to fill up 256 blocks of memory
  for (int block = InitRAMContent.size(); block < 256; ++block)
    InitRAMContent.push_back(0);

  // Notify the user about variables automatically added (if have)
  if (variableNames.size() > 0)
    cout << "[debug] Added variables: " << endl;
  for (unsigned int iName = 0; iName < variableNames.size(); ++iName)
    cout << "    [+] " << variableNames[iName] << ": " << toBinaryString(variableMap[variableNames[iName]], 8) << " (" << variableMap[variableNames[iName]] << ")" << endl;
  return true;
}

bool compileCodeFile(string filename, vector<int>& InitRAMContent) {
  cout << "[debug] Compiling the code..." << endl;

  /* Code file */
  fstream codeFile;

  /* Map of variable names -> 8-bit addresses */
  map<string, int> variableMap;

  codeFile.open(filename, fstream::in);
  if (!codeFile) {
    cout << "[error] No such file \"" << filename << "\" is found." << endl;
    return false;
  }

  // Convert tag into addresses
  if (!compileTags(codeFile, variableMap))
    return false;

  codeFile.clear();
  codeFile.seekg(0);

  // Put code -> RAM;
  // Convert variable names into addresses
  if (!compileInstructions(codeFile, variableMap, InitRAMContent))
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//                                   WRITE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////

bool writeInitRAMToFile(vector<int> InitRAMContent, string outputName) {
  fstream outputFile;

  outputFile.open(outputName, fstream::out);
  if (!outputFile) {
    cout << "[error] Cannot write to file \"" << outputName << "\". Permission Denied?" << endl;
    return false;
  }

  for (unsigned int i = 0; i < InitRAMContent.size(); ++i) {
    string binData = toBinaryString(InitRAMContent[i], 8);
    string binLine = toBinaryString(i, 8);
    outputFile << binLine << " | " << binData << endl;
  }

  return true;
}

string getOutputName(string inputName) {
  if (inputName.rfind(".") != string::npos) {
    unsigned int place = inputName.rfind(".");
    inputName.erase(place);
  }
  return inputName += ".out";
}

//////////////////////////////////////////////////////////////////////////////////////////////
//                                        MAIN
//////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  initGlobal();

  if (argc <= 1) {
    cout << "[usage] " << argv[0] << " <Source.su>" << endl;
    cout << "[error] No arguments are given to the program!" << endl;
    return 0;
  }

  for (int i = 1; i < argc; ++i) {
    vector<int> InitRAMContent;
    string assemblyCodeFileName_In;
    string machineCodeFileName_Out;

    assemblyCodeFileName_In = string(argv[i]);
    machineCodeFileName_Out = getOutputName(assemblyCodeFileName_In);

    if (compileCodeFile(assemblyCodeFileName_In, InitRAMContent))
      writeInitRAMToFile(InitRAMContent, machineCodeFileName_Out);
  }
}
