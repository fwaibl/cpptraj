#include "SymmetricRmsdCalc.h"
#include "DistRoutines.h"
#include "AtomMap.h"
#include "CpptrajStdio.h"

SymmetricRmsdCalc::SymmetricRmsdCalc() : debug_(3) {}

/** Find potential symmetric atoms. All residues up to the last selected
  * residue are considered, but all atoms within those residues (even
  * unselected ones) because when symmetric atoms are re-mapped, atoms
  * bonded to the symmetric atoms (which are themselves symmetric) need
  * to be re-mapped as well.
  */
int SymmetricRmsdCalc::FindSymmetricAtoms(Topology const& topIn, AtomMask const& tgtMask) {
  // Allocate space for selected atoms in the target frame. This will also
  // put the correct masses in based on the mask.
  tgtFrame_.SetupFrameFromMask(tgtMask, topIn.Atoms());
  // Allocate space for remapped frame; same # atoms as original frame
  remapFrame_.SetupFrameV( topIn.Atoms(), topIn.HasVelInfo(), topIn.NrepDim() );
  // Create initial 1 to 1 atom map for all atoms; indices in 
  // SymmetricAtomIndices will correspond to positions in AMap.
  AMap_.clear();
  AMap_.reserve( topIn.Natom() );
  for (int atom = 0; atom < topIn.Natom(); atom++)
    AMap_.push_back(atom);
  // Determine last selected residue.
  int last_res = topIn[tgtMask.back()].ResNum() + 1;
  mprintf("\tResidues up to %s will be considered.\n", topIn.TruncResNameNum(last_res).c_str());
  // In each residue, determine which atoms are symmetric.
  SymmetricAtomIndices_.clear();
  AtomMap resmap;
  if (debug_ > 1) resmap.SetDebug(1);
  for (int residue = 0; residue < last_res; ++residue) {
    int res_first_atom = topIn.Res(residue).FirstAtom();
    if (debug_ > 0)
      mprintf("DEBUG: Residue %s\n", topIn.TruncResNameNum(residue).c_str());
    if (resmap.SetupResidue(topIn, residue) != 0) return 1;
    if (resmap.CheckBonds() != 0) return 1;
    resmap.DetermineAtomIDs();
    // Get indices of potentially symmetric atoms for this residue.
    // NOTE: Indices for resmap start at 0.
    // AtomStatus: 0=Unselected, 1=Selected/Non-symm., 2=Selected/Symm
    enum atomStatusType { UNSELECTED = 0, NONSYMM, SYMM };
    std::vector<int> AtomStatus(resmap.Natom(), UNSELECTED);
    for (int atom1 = 0; atom1 < resmap.Natom(); atom1++) {
      int actual_atom1 = atom1 + res_first_atom; // Actual atom index in topIn
      if (AtomStatus[atom1] == UNSELECTED) {
        AtomStatus[atom1] = NONSYMM; // Initially select as non-symmetric
        // Check if atom1 is duplicated and not bound to a chiral center. 
        // If so, find all selected duplicates.
        if (!resmap[atom1].BoundToChiral() && resmap[atom1].Nduplicated() > 0) {
          AtomStatus[atom1] = SYMM; // Select atom1 as symmetric
          Iarray symmatoms(1, actual_atom1);
          for (int atom2 = atom1 + 1; atom2 < resmap.Natom(); atom2++) {
            int actual_atom2 = atom2 + res_first_atom;
            // Check if atom2 matches atom1 and is not bound to a chiral center
            if (resmap[atom1].Unique() == resmap[atom2].Unique() &&
                !resmap[atom2].BoundToChiral())
            {
              AtomStatus[atom2] = SYMM; // Select atom2 as symmetric with atom1
              symmatoms.push_back(actual_atom2);
            }
          } // END loop over atom2
          if (debug_ > 0)
            mprintf("DEBUG:\t\tAtom %s ID %s is duplicated %u times:",
                    topIn.TruncResAtomName(symmatoms.front()).c_str(),
                    resmap[atom1].Unique().c_str(), symmatoms.size());
          if (symmatoms.size() > 1) {
            SymmetricAtomIndices_.push_back( symmatoms );
            if (debug_ > 0)
              for (Iarray::const_iterator sa = symmatoms.begin(); sa != symmatoms.end(); ++sa)
                mprintf(" %s", topIn.AtomMaskName(*sa).c_str());
          } else {
            // Only one atom selected, no symmetry. Change to non-symmetric.
            // FIXME: Does this ever occur now that we always select all for symmetry?
            AtomStatus[symmatoms.front() - res_first_atom] = NONSYMM; // Select as non-symmetric
          }
          if (debug_ > 0) mprintf("\n");
        } // END if atom is duplicated
      }
    } // END loop over atom1
    // TODO: If fitting, set up mask to perform initial fit with selected nonsymmetric atoms
    if (debug_ > 0) {
      mprintf("DEBUG:\tSelected Non-symmetric atoms:");
      for (int atom1 = 0; atom1 < resmap.Natom(); atom1++)
        if (AtomStatus[atom1] == NONSYMM) { // If selected/non-symmetric
          mprintf(" %s", topIn.AtomMaskName(atom1 + res_first_atom).c_str());
          //InitialFitMask.AddAtom(atom1 + res_first_atom);
        }
      mprintf("\n");
    }
  } // End loop over residues
  if (debug_ > 0) {
    mprintf("DEBUG: Symmetric Atom Groups:\n");
    for (AtomIndexArray::const_iterator symmatoms = SymmetricAtomIndices_.begin();
                                        symmatoms != SymmetricAtomIndices_.end();
                                        ++symmatoms)
    {
      mprintf("\t");
      for (Iarray::const_iterator atom = symmatoms->begin();
                                  atom != symmatoms->end(); ++atom)
        mprintf(" %s", topIn.AtomMaskName(*atom).c_str());
      mprintf("\n");
    } 
  }
  return 0;
}

// SymmetricRmsdCalc::SymmRMSD()
double SymmetricRmsdCalc::SymmRMSD(Frame const& TGT, AtomMask const& tgtMask,
                                   Frame const& REF, Frame const& centeredREF, 
                                   Matrix_3x3& rot, Vec3& tgtTrans,
                                   Vec3 const& refTrans, bool fit, bool useMass)
{
  tgtFrame_.SetCoordinates(TGT, tgtMask);
  // Calculate initial best fit RMSD if necessary
  if (fit) {
    tgtFrame_.RMSD_CenteredRef(centeredREF, rot, tgtTrans, useMass);
    // tgtFrame has been translated to origin but not rotated and translated to ref
    tgtFrame_.Trans_Rot_Trans(Vec3(0.0), rot, refTrans);
  }
  // Correct RMSD for symmetry
  for (AtomIndexArray::const_iterator symmatoms = SymmetricAtomIndices_.begin();
                                      symmatoms != SymmetricAtomIndices_.end(); ++symmatoms)
  {
    // For each array of symmetric atoms, determine the lowest distance score
    cost_matrix_.Initialize( symmatoms->size() );
    for (Iarray::const_iterator tgtatom = symmatoms->begin();
                                tgtatom != symmatoms->end(); ++tgtatom)
    {
      for (Iarray::const_iterator refatom = symmatoms->begin();
                                  refatom != symmatoms->end(); ++refatom)
      {
        double dist2 = DIST2_NoImage( REF.XYZ(*refatom), TGT.XYZ(*tgtatom) );
        mprintf("\t\t%i to %i: %f\n", *tgtatom + 1, *refatom + 1, dist2);
        cost_matrix_.AddElement( dist2 );
      }
    }
//    mprintf("\tSymmetric atoms starting with %i", (*symmatoms).front() + 1);
    Iarray resMap = cost_matrix_.Optimize();
    // Fill in overall map
    Iarray::const_iterator rmap = resMap.begin();
    for (Iarray::const_iterator atmidx = symmatoms->begin();
                                atmidx != symmatoms->end(); ++atmidx, ++rmap)
    {
      AMap_[*atmidx] = (*symmatoms)[*rmap]; // FIXME: Check indices
//      mprintf("\tAssigned atom %i to atom %i\n", *atmidx + 1, (*symmatoms)[*rmap] + 1);
    }
  }
  for (unsigned int ref = 0; ref < AMap_.size(); ++ref)
    mprintf("\t%u -> %i\n", ref + 1, AMap_[ref] + 1);
  double rmsdval;
  // FIXME: Check that masses are also remapped
  if (fit) {
    // Use the initial input frame coordinates (prior to the fit)
    // FIXME: Use SetTarget/SetRefByMap??
    remapFrame_.SetCoordinatesByMap(TGT, AMap_);
    tgtFrame_.SetCoordinates(remapFrame_, tgtMask);
    rmsdval = tgtFrame_.RMSD_CenteredRef( centeredREF, rot, tgtTrans, useMass );
  } else {
    remapFrame_.SetCoordinatesByMap(TGT, AMap_);
    rmsdval = remapFrame_.RMSD_NoFit(centeredREF, useMass); 
  }
  return rmsdval;
} 
