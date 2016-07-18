#ifndef Soda_h
#define Soda_h

#include "application.h"
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

class Soda {
  public:
    Soda();
    ~Soda();
      void pins(int a, int b, int c, int d, int e, int f, int g, int dp);
      void write(int number);
      void clear();
      void increment();
      void decrement();

  private:
      byte numeral[10];
      int segmentPins[8];
      int _number;
};

#endif
