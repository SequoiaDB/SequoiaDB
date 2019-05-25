/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

const CHAR *pdGetTraceComponent ( UINT32 id )
{
   if ( (INT32)id >= _pdTraceComponentNum )
   {
      return NULL ;
   }
   return _pdTraceComponentDir[id] ;
}
