/******************************************************************************
    (c) 2002 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

#include <string>
#include "interfaces.h"

#ifdef __linux__
#include "interfaces-linux.h"
#endif


LATinterfaces::LATinterfaces()
{
}

LATinterfaces::~LATinterfaces()
{
}

// LATinterfaces *LATinterfaces::Create() is in
// a real implementation class.
