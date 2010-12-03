#ifndef INC_CONFLIB_H
#define INC_CONFLIB_H
// Conflib
//#include <cstdio>
#include "TrajFile.h"

class Conflib: public TrajFile {
    double energy;
    double radGyr;
    int timesFound;

  public:

    Conflib();
    ~Conflib();

    int open();
    void close();
    int getFrame(int);
    int SetupRead();
    int SetupWrite();
    int writeFrame(int);
    void Info();   
};
#endif
