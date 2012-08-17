#ifndef INC_TRAJ_AMBERCOORD_H
#define INC_TRAJ_AMBERCOORD_H
#include "TrajectoryIO.h"
#include "FrameBuffer.h"
// Class: Traj_AmberCoord
/// Reads and writes formatted (ASCII text) amber trajectories. 
class Traj_AmberCoord: public TrajectoryIO, FrameBuffer {
  public:
    Traj_AmberCoord();
    ~Traj_AmberCoord();
    // AmberCoord-specific functions
    void SetHighPrecision();
  private:
    int natom3_;         ///< Number of coords (# atoms X 3)
    size_t titleSize_;   ///< Title size in bytes
    size_t hasREMD_;     ///< REMD header size if present
    int numBoxCoords_;   ///< Number of box coords, 3 (ortho or truncoct) or 6 (triclinic)
    const char* outfmt_; ///< Format string for writing coordinates
    bool highPrecision_; ///< If true output format will be 8.6 instead of 8.3
    TrajectoryIO* mdvel_;

    static const size_t REMD_HEADER_SIZE;
    static const size_t BUF_SIZE;

    // Inherited functions
    bool ID_TrajFormat();
    int setupTrajin(Topology*);
    int setupTrajout(Topology*,int);
    int openTraj();
    void closeTraj();
    int readFrame(int,double*,double*,double*,double*);
    int writeFrame(int,double*,double*,double*,double);
    void info();
    int processWriteArgs(ArgList*);
    int processReadArgs(ArgList&);
};
#endif
