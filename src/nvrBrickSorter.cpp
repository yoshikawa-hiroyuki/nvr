/*
 * 4DVis - 4D Visualization system of RIKEN -
 *
 * Copyright (c) The Institute of Physical and Chemical Research, 2000-2002
 *           All right reserved.
 */

#include "nvrBrickSorter.h"

using namespace CES;


//-----------------------------------------------------------
//  SortByError : sorting by residual error
//-----------------------------------------------------------
void nvrBrickSorter::SortByError(std::deque<nvrBrick*>& bl,
                                 const NVR::ErrLevelType stype)
{
  if ( bl.size() < 2 ) return;
  if ( stype == NVR::TEST_LEVEL )
    std::sort(bl.begin(), bl.end(), ErrCompTest);
  else
    std::sort(bl.begin(), bl.end(), ErrCompCurrent);
}

//-----------------------------------------------------------
//  SortByPriority : sorting by priority
//-----------------------------------------------------------
void nvrBrickSorter::SortByPriority(std::deque<nvrBrick*>& bl)
{
  if ( bl.size() < 2 ) return;
  std::sort(bl.begin(), bl.end(), PrioComp);
}
