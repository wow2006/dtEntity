#pragma once

/* -*-c++-*-
* dtEntity Game and Simulation Engine
*
* This library is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Martin Scheffler
*/

#include <dtEntity/export.h>

// comment this out to make debug and release mode behave the same:
// strings are stored as integers instead of std::strings.
//#define DTENTITY_USE_STRINGS_AS_STRINGIDS

#include <string>

namespace dtEntity
{
   // serial ID for entities
   typedef unsigned int EntityId;

   // serial ID for strings

#if defined(DTENTITY_USE_STRINGS_AS_STRINGIDS)
   typedef std::string StringId;
#else
   typedef unsigned int StringId;
#endif

   // type id for messages
   typedef StringId MessageType;

   // type id for components
   typedef StringId ComponentType;
   
}
