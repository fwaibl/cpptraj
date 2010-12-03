// Conflib
#include <cstdlib>
#include "Conflib.h"

// CONSTRUCTOR
Conflib::Conflib() { 
  energy=0.0;
  radGyr=0.0;
  timesFound=0;
}

// DESTRUCTOR
Conflib::~Conflib() { 
  //fprintf(stderr,"Conflib Destructor.\n");
}
//------------------------------------------------------------------------
/* Conflib::close()
 */
void Conflib::close() {
  File->CloseFile();
}

/* Conflib::open()
 */
int Conflib::open() {

  if (File->OpenFile()) return 1;

  return 0;
}


/* Conflib::SetupRead()
 */
int Conflib::SetupRead() {
  long unsigned int confFrame;

  //if ( this->open() ) return 1;

  // Conflib is double,double,int,natom*3*double
  confFrame = (((P->natom * 3) + 2) * sizeof(double)) + sizeof(int);

  if ( (File->frame_stat.st_size % confFrame) == 0 ) {
    Frames = (int) (File->frame_stat.st_size / confFrame);
    stop = Frames;
  } else {
    fprintf(stdout,"Warning: Conflib::SetupRead(): Could not predict # frames\n");
    fprintf(stdout,"         Ensure that associated parm has correct # atoms.\n");
    fprintf(stdout,"         File size=%lu confFrame=%lu\n",File->frame_stat.st_size,
            confFrame);
    Frames=-1;
    stop=-1;
    return 1;
  }

  //this->close();

  return 0;
}

/* Conflib::getFrame()
 */
int Conflib::getFrame(int set) {

  if (File->IO->Read(&energy,sizeof(double),1) < 0) return 1;
  File->IO->Read(&radGyr,sizeof(double),1);
  File->IO->Read(&timesFound,sizeof(int),1);
  File->IO->Read(F->X,sizeof(double),P->natom*3); 

  return 0;
}

// Set up trajectory for either write or append
int Conflib::SetupWrite( ) {
  fprintf(stdout,"Error: conflib writes not yet implemented.\n");
  return 1;
}

// Write a frame
int Conflib::writeFrame(int set) {
  fprintf(stdout,"Error: conflib writes not yet implemented.\n");
  return 1;
}

/*
 * Info()
 */
void Conflib::Info() {
  fprintf(stdout,"  File (%s) is an LMOD conflib file", File->filename);
/*    if (p->option2 == 1) 
      printfone(" with no atom wrapping");
    if (p->option1 == 1)
      printfone(": AMBER charges and radii in prmtop to occupancy and temp factor columns");
    else if (p->option1 == 2)
      printfone(": AMBER charges and PARSE radii to occupancy and temp factor columns");
    else if (p->option1 == 3)
      printfone(": AMBER charges and vdw radii (r*) to occupancy and temp factor columns");*/
}
