#ifndef INC_PARM_CIF_H
#define INC_PARM_CIF_H
#include "ParmIO.h"
class Parm_CIF : public ParmIO {
  public :
    Parm_CIF() : debug_(0) { }
    static BaseIOtype* Alloc() { return (BaseIOtype*)new Parm_CIF(); }
    bool ID_ParmFormat(CpptrajFile&);
    int processReadArgs(ArgList&) { return 0; }
    int ReadParm(FileName const&, Topology&);
    int WriteParm(FileName const&, Topology const&) { return 1;   }
    void SetDebug(int i)                               { debug_ = i; }
    int processWriteArgs(ArgList&) { return 0; }
    bool NeedsBondSearch() const { return true; }
  private:
    enum EntryType { ANAME=0, RNAME, X, Y, Z, RNUM, CHAINID, NENTRY };
    static const char* Entries[];
    int debug_;
};
#endif
