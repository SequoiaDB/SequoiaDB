/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = pdComponents.cpp

   Descriptive Name = Problem Determination

   When/how to use: this program may be used on binary and text-formatted
   versions of PD component. This file contains functions for components

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pdTrace.hpp"

// Whenever we add a new component, we have to modify
// 1) pdTrace.hpp ( the component list )
// 2) pdComponents.cpp ( component list string )
// 3) pdTrace.hpp ( _pdTraceComponentNum )
// 4) add a new directory with EXACT same component name
// 5) create <comp>Trace.hpp which will include <comp>Trace.h
// There will be <comp>Trace.h automatically created in include directory
// Note there are max 32 components can be added into list
const CHAR *_pdTraceComponentDir[] = {
   "auth",   // PD_TRACE_COMPONENT_AUTH
   "bps",    // PD_TRACE_COMPONENT_BPS
   "cat",    // PD_TRACE_COMPONENT_CAT
   "cls",    // PD_TRACE_COMPONENT_CLS
   "dps",    // PD_TRACE_COMPONENT_DPS
   "mig",    // PD_TRACE_COMPONENT_MIG
   "msg",    // PD_TRACE_COMPONENT_MSG
   "net",    // PD_TRACE_COMPONENT_NET
   "oss",    // PD_TRACE_COMPONENT_OSS
   "pd",     // PD_TRACE_COMPONENT_PD
   "rtn",    // PD_TRACE_COMPONENT_RTN
   "sql",    // PD_TRACE_COMPONENT_SQL
   "tools",  // PD_TRACE_COMPONENT_TOOL
   "bar",    // PD_TRACE_COMPONENT_BAR
   "client", // PD_TRACE_COMPONENT_CLIENT
   "coord",  // PD_TRACE_COMPONENT_COORD
   "dms",    // PD_TRACE_COMPONENT_DMS
   "ixm",    // PD_TRACE_COMPONENT_IXM
   "mon",    // PD_TRACE_COMPONENT_MON
   "mth",    // PD_TRACE_COMPONENT_MTH
   "opt",    // PD_TRACE_COMPONENT_OPT
   "pmd",    // PD_TRACE_COMPONENT_PMD
   "rest",   // PD_TRACE_COMPONENT_REST
   "spt",    // PD_TRACE_COMPONENT_SPT
   "util",   // PD_TRACE_COMPONENT_UTIL
   "aggr",   // PD_TRACE_COMPONENT_AGGR
   "spd",    // PD_TRACE_COMPONENT_SPD
   "qgm"     // PD_TRACE_COMPONENT_QGM
} ;

UINT32 pdGetTraceComponentSize()
{
   return sizeof( _pdTraceComponentDir ) / sizeof( const CHAR* ) ;
}

const CHAR *pdGetTraceComponent ( UINT32 id )
{
   if ( id >= pdGetTraceComponentSize() )
   {
      return NULL ;
   }
   return _pdTraceComponentDir[id] ;
}
