#include <iostream>

using namespace std;

void outputBinary(uint16_t number) {
  for (int i = 15; i >= 0; --i) {
    cout << (((number >> i) & 0x1) ? "1" : "0");
  }
}

int main() {
  uint8_t a = 10;
  uint8_t b = 1;
  uint16_t result = a + (~b & 0b11111111) + 1;
  
  outputBinary(result);
}
