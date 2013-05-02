#include <cmath> // exp
#include <algorithm> // std::max_element
#include "Action_Grid.h"
#include "CpptrajStdio.h"
#include "PDBfile.h"

// CONSTRUCTOR
Action_Grid::Action_Grid() :
  max_(0.80),
  madura_(0),
  smooth_(0),
  invert_(false),
  grid_(0)
{}

void Action_Grid::Help() {
  mprintf("\t<filename> %s <mask>\n", GridAction::HelpText);
  mprintf("\t[max <fraction>] [smoothdensity <value>] [invert] [madura <madura>]\n");
  mprintf("\t[pdb <pdbout>] [opendx]\n");
  mprintf("\tBin atoms in <mask> into a 3D grid.\n");
  mprintf("\t<fraction>: Percent of max to write.\n");
  mprintf("\t<madura>  : Grid values lower than <madura> become flipped in sign, exposes low density.\n");
  mprintf("\t<value>   : Used to smooth density.\n");
  mprintf("\t[opendx]  : Write the density file in OpenDX format.\n");
}

// Action_Grid::Init()
Action::RetType Action_Grid::Init(ArgList& actionArgs, TopologyList* PFL, FrameList* FL,
                          DataSetList* DSL, DataFileList* DFL, int debugIn)
{
  // Get output filename
  std::string filename = actionArgs.GetStringNext();
  if (filename.empty()) {
    mprinterr("Error: GRID: no filename specified.\n");
    return Action::ERR;
  }
  // Get grid options
  grid_ = GridInit( "GRID", actionArgs, *DSL );
  if (grid_ == 0) return Action::ERR;

  // Get extra options
  max_ = actionArgs.getKeyDouble("max", 0.80);
  madura_ = actionArgs.getKeyDouble("madura", 0);
  smooth_ = actionArgs.getKeyDouble("smoothdensity", 0);
  invert_ = actionArgs.hasKey("invert");
  if (actionArgs.hasKey("opendx")) 
    mprintf("Warning: 'opendx' is deprecated.\n");
  pdbname_ = actionArgs.GetStringKey("pdb"); 

  // Get mask
  std::string maskexpr = actionArgs.GetMaskNext();
  if (maskexpr.empty()) {
    mprinterr("Error: GRID: No mask specified.\n");
    return Action::ERR;
  }
  mask_.SetMaskString(maskexpr);

  // Setup output file
  DataFile* outfile = DFL->AddSetToFile(filename, (DataSet*)grid_);
  if (outfile == 0) {
    mprinterr("Error: grid: Could not set up output file %s\n", filename.c_str());
    return Action::ERR;
  }

  // Info
  mprintf("    GRID:\n");
  GridInfo( *grid_ );
  mprintf("\tGrid will be printed to file %s\n",filename.c_str());
  mprintf("\tMask expression: [%s]\n",mask_.MaskString());
  if (pdbname_.empty())
    mprintf("\tPseudo-PDB will be printed to STDOUT.\n");
  else
    mprintf("\tPseudo-PDB will be printed to %s\n", pdbname_.c_str());
  // TODO: print extra options

  return Action::OK;
}

// Action_Grid::Setup()
Action::RetType Action_Grid::Setup(Topology* currentParm, Topology** parmAddress) {
  // Setup grid, checks box info.
  if (GridSetup( *currentParm )) return Action::ERR;

  // Setup mask
  if (currentParm->SetupIntegerMask( mask_ ))
    return Action::ERR;
  mask_.MaskInfo();
  if (mask_.None()) {
    mprinterr("Error: GRID: No atoms selected for parm %s\n", currentParm->c_str());
    return Action::ERR;
  }

  return Action::OK;
}

// Action_Grid::DoAction()
Action::RetType Action_Grid::DoAction(int frameNum, Frame* currentFrame, Frame** frameAddress) {
  GridFrame( *currentFrame, mask_, *grid_ );
  return Action::OK;
}

// Action_Grid::print()
void Action_Grid::Print() {
  // DEBUG
  //mprintf("CDBG: Printing grid.\n");
  //PrintEntireGrid();
  // END DEBUG

  // Perform normalization and find max
  double gridMax = 0;
  for (DataSet_GridFlt::iterator gval = grid_->begin(); gval != grid_->end(); ++gval) {
    double gridval = (double)(*gval);
    // ----- SMOOTHING -----
    if (smooth_ > 0.0) {
      double yy = gridval - smooth_;
      double xx = yy*yy / (0.2 * smooth_ * smooth_);
      xx = exp( -xx );
      if (invert_) {
        if (gridval > smooth_) // NOTE: Comparison OK? Needs cast?
          gridval = -5.0;
        else
          gridval -= gridval * xx;
        /* COMMENTED OUT IN ORIGINAL PTRAJ CODE
        if (gridInfo->grid[index] < action->darg3) {
          gridInfo->grid[index] = 0.0;
        }
        */
        if (gridval >= 0)
          gridval = smooth_ - gridval;
      } else {
        if (gridval < smooth_)
          gridval = 0;
        else
          gridval -= gridval * xx;
        if (gridval < smooth_)
          gridval = 0;
      }
    }

    // do the madura negative option to expose low density
    if ( madura_ > 0.0 && gridval > 0.0 && gridval < madura_ )
      *gval = (float)-gridval;
    else
      *gval = (float) gridval;

    if ( gridval > gridMax )
      gridMax = gridval;
  } 

  // PDBfile output
  mprintf("    GRID: grid max is %.3lf\n", gridMax);
  PrintPDB( gridMax );
}

// Action_Grid::PrintPDB()
void Action_Grid::PrintPDB(double normIn)
{
  double norm = normIn;
  // Calculate normalization if necessary
  if (norm <= 0) {
    norm = (double)*std::max_element(grid_->begin(), grid_->end());
    if (norm == 0) {
      mprinterr("Error: Grid max is 0. No density for PDB write.\n");
      return;
    }
    mprintf("\t%s: Normalizing grid by %f\n", norm);
  }
  norm = 1.0 / norm;
  // Write PDB
  PDBfile pdbout;
  if (pdbout.OpenWrite(pdbname_)) {
    mprinterr("Error: Cannot open PDB for grid output.\n");
    return;
  }
  mprintf("\tWriting PDB of grid points > %.3f of grid max.\n", max_);
  int res = 1;
  for (size_t k = 0; k < grid_->NZ(); ++k) {
    for (size_t j = 0; j < grid_->NY(); ++j) {
      for (size_t i = 0; i < grid_->NX(); ++i) {
        double gridval = grid_->GetElement(i, j, k) * norm;
        if (gridval > max_) {
          Vec3 cxyz = grid_->BinCenter(i,j,k);
          pdbout.WriteATOM(res++, cxyz[0], cxyz[1], cxyz[2], "GRID", gridval);
        }
      }
    }
  }
  // Write grid boundaries
  for (size_t k = 0; k <= grid_->NZ(); k += grid_->NZ())
    for (size_t j = 0; j <= grid_->NY(); j += grid_->NY())
      for (size_t i = 0; i <= grid_->NX(); i += grid_->NX()) {
        Vec3 cxyz = grid_->BinCenter(i,j,k);
        pdbout.WriteHET(res, cxyz[0], cxyz[1], cxyz[2]);
      }
}
