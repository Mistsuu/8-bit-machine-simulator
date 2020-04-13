#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
using namespace std;

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

map<string, int> code;

inline bool isInt(string s) {
  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] < '0' || s[i] > '9') return false;
  }
  return true;
}

inline bool variableAvailable(string varName, map<string, int> variableList) {
  return !(variableList.find(varName) == variableList.end());
}

inline bool isInstruction(string opcode) {
  return !(code.find(opcode) == code.end());
}

inline bool isTag(string tagName) {
  return (tagName[tagName.length() - 1] == ':');
}

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

template <typename T>
inline string getBinary(T data, int bits) {
  string binData;
  for (; bits > 0; --bits) {
    binData += (((data >> (bits - 1)) & 0x1) ? "1" : "0");
  }
  return binData;
}

inline void generateVariable(string varName, unsigned int &stackReg, map<string, int> &variableList) {
  variableList[varName] = stackReg--;
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

bool compile(string filename, vector<int> &RAMContent) {
  cout << "[Debug: ] Compiling the code..." << endl;

  /* String parser */
  stringstream ssin;  // Parse int & strings
  fstream codeFile;   // Have code file
  string  codeLine;   // One code line

  /* Part of code */
  string  opcode;     // Opcode
  string  argument;   // Argument
  string  tag;        // Tag

  /* Store data about the program */
  unsigned int stackReg = 0xff;   // Store variables created in memory
  map<string, int> variableList;  // List of variables used
  vector<string> variableNames;   // List of variables' name
  vector<string> tagNames;        // List of tags' name
  unsigned int tagPlace = 0;

  // Open file
  codeFile.open(filename, fstream::in);
  if (!codeFile) {
    cout << "[Error: ] No such file is found." << endl;
    return false;
  }

  // Setting up tag
  while (getline(codeFile, codeLine)) {
    filterComment(codeLine);     // Get rid of comments

    opcode = "";
    argument = "";

    ssin.clear();
    ssin.str(codeLine);

    ssin >> opcode;
    ssin >> argument;

    if (opcode == "") continue;

    if (isTag(opcode)) { /* Idenify as a tag */
      tag = opcode.substr(0, opcode.length() - 1);
      if (variableAvailable(tag, variableList)) {
        cout << "[Error: ] Tag conflict with another tag!" << endl;
        return false;
      }

      if (argument != "") {
        cout << "[Error: ] Syntax Error For Tags."  << endl;
        return false;
      }

      variableList[tag] = tagPlace;
      tagNames.push_back(tag);
      continue;
    }

    if (opcode   != "") tagPlace++;
    if (argument != "") tagPlace++;

  }

  if (tagNames.size() > 0)
    cout << "[Debug: ] Added following tags..." << endl;
  for (unsigned int iName = 0; iName < tagNames.size(); ++iName) {
    cout << "    [+] " << tagNames[iName] << ": " << getBinary(variableList[tagNames[iName]], 8) << endl;
  }




  // Very important, seek again to the begin of file
  codeFile.clear();
  codeFile.seekg(0);




  // Getting data
  while (getline(codeFile, codeLine)) {
    filterComment(codeLine);     // Get rid of comments

    opcode = "";
    argument = "";

    ssin.clear();
    ssin.str(codeLine);

    ssin >> opcode;
    ssin >> argument;

    if (opcode == "") continue;

    cout << opcode << " " << argument << endl;

    if (isTag(opcode)) continue; // Skip tags
    else if (!isInstruction(opcode)) {
      cout << "[Error: ] Instruction not recognized." << endl;
      return false;
    }
    else RAMContent.push_back(code[opcode] << 4);

    switch (code[opcode]) {
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
          cout << "[Error: ] Instruction Argument Required." << endl;
          return false;
        }

        /* If Argument Is A Variable String, Convert To Memory Then Add To RAM */
        if (!isInt(argument)) {

          // New Variable
          if (!variableAvailable(argument, variableList)) {

            // Check Overflow
            if (stackReg <= RAMContent.size()) {
              cout << "[Error: ] Memory Overflow Detected." << endl;
              return false;
            }

            // Generate
            generateVariable(argument, stackReg, variableList);
            variableNames.push_back(argument);
          }

          // Add to RAM
          RAMContent.push_back(variableList[argument]);
        }

        /* Else, Just Add to RAM Normally */
        else {
          RAMContent.push_back(toInteger(argument));
        }
        break;

      case NOP:
      case HLT:
      case OUT:
      case SLF:
        if (argument != "") {
          cout << "[Error: ] Parsed An Argument For A Command That Doesn't Need One." << endl;
          return false;
        }
        break;
    }

  }

  // Add the Remaining memory space to fill up 256 blocks of memory
  for (int block = RAMContent.size(); block < 256; ++block) {
    RAMContent.push_back(0);
  }

  // Notify the user about variables automatically added (if have)
  if (variableNames.size() > 0)
    cout << "[Debug: ] Added variables: " << endl;
  for (unsigned int iName = 0; iName < variableNames.size(); ++iName) {
    cout << "    [+] " << variableNames[iName] << ": " << getBinary(variableList[variableNames[iName]], 8) << endl;
  }

  return true;
}


bool write(vector<int> RAMContent, string outputName) {
  fstream outputFile;

  outputFile.open(outputName, fstream::out);
  if (!outputFile) {
    cout << "[Error: ] Cannot write to file \"" << outputName << "\". Permission Denied?" << endl;
    return false;
  }

  for (unsigned int i = 0; i < RAMContent.size(); ++i) {
    string binData = getBinary(RAMContent[i], 8);
    string binLine = getBinary(i, 8);
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

int main(int argc, char *argv[]) {
  mapOPCode();

  if (argc <= 1) {
    cout << "[Error: ] No Arguments Given For The Program!" << endl;
    return 0;
  }

  for (int i = 1; i < argc; ++i) {
    vector<int> RAMContent;
    string assemblyCodeFileName_In;
    string machineCodeFileName_Out;

    assemblyCodeFileName_In = string(argv[i]);
    machineCodeFileName_Out = getOutputName(assemblyCodeFileName_In);

    if (compile(assemblyCodeFileName_In, RAMContent)) {
      write(RAMContent, machineCodeFileName_Out);
    }
  }
}
