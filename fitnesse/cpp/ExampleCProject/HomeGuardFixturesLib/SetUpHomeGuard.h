//
// Copyright (c) 2004 James Grenning
// Released under the terms of the GNU General Public License version 2 or later.
//

#ifndef SETUP_H
#define SETUP_H

#include "Fit/ColumnFixture.h"

extern "C"
{
#include "MockFrontPanel.h"
}

class SetUpHomeGuard : public Fixture 
{
public:
  SetUpHomeGuard();
  static FrontPanel* GetFrontPanel();

};


#endif
