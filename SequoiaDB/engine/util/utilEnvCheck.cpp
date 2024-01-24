/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilEnvCheck.cpp

   Descriptive Name =

   When/how to use: linux environment check util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== =========== ==============================================
          20/04/2016  Chen Chucai Initial Draft

   Last Changed =

******************************************************************************/

#include "utilEnvCheck.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

#include "ossNPipe.hpp"
#include "ossIO.hpp"

#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <string>
#include <vector>

using std::string ;
using std::vector ;

namespace engine
{
   #define UTIL_VM_MINFREE_KBYTES_PERCENTAGE 8

   static BOOLEAN _isIdealValue( const string &result,
                                 const string &idealValue,
                                 const string &seperator1,
                                 const string &seperator2,
                                 const string &path ) ;

   static BOOLEAN _thpCheck( const string &path ) ;

   static INT64 _utilGetSysMemTotalVolKB();

   BOOLEAN utilCheckIs64BitSys()
   {//return TRUE means system is 64bit, suggest run sequoiadb on 64 bit system
      BOOLEAN is64bit = TRUE ;
      ossOSInfo info ;

      ossGetOSInfo( info ) ;
      if( info._bit != 64 )
      {
         PD_LOG( PDWARNING, "Your system is not 64 bit."
                 " Even though it will still work, but SequoiaDB suggest run"
                 " SequoiaDB on 64 bit system. " ) ;
         is64bit = FALSE ;
      }

      return is64bit ;
   }

   BOOLEAN utilCheckIsOpenVZ()
   {//return TRUE means system is running on openVZ,
    //suggest DO NOT run sequoiadb on openVZ
      BOOLEAN isOpenVZ = FALSE ;

      if( ( SDB_OK == ossAccess( "/proc/vz" ) ) &&
          ( SDB_OK != ossAccess( "/proc/bc" ) ) )
      {
         PD_LOG( PDWARNING, "Your system is running on openVZ. "
                 "Even though it will still work, but SequoiaDB "
                 "suggest not to do that. " ) ;
         isOpenVZ = TRUE ;
      }

      return isOpenVZ ;
   }

   BOOLEAN utilCheckNumaStatus()
   {// return TRUE means numa is in use, suggest not use numa
      BOOLEAN isNumaInUse = FALSE ;
      std::fstream f;

      if( SDB_OK != ossAccess( "/sys/devices/system/node/node1" ) )
      {
         goto done ;
      }

      /*
       * /proc/self/numa_maps: the process's numa_maps who is accessing the file
      */
      f.open( "/proc/self/numa_maps", std::ios_base::in ) ;
      if( f.is_open() )
      {
         std::string::size_type spaceLoc ;
         std::string::size_type iLoc ;
         std::string line ;
         const char space        = ' ';
         const char interleave[] = "interleave";

         std::getline( f, line ) ;
         if( f.fail() )
         {
            PD_LOG( PDWARNING, "Failed to read from /proc/self/numa_maps") ;
            isNumaInUse = TRUE ;

            goto done ;
         }

         spaceLoc = line.find( space ) ;
         iLoc = spaceLoc + 1 ;
         if( ( spaceLoc == std::string::npos ) || ( iLoc == line.size() ) )
         {
            PD_LOG( PDWARNING, "Can not parse numa_maps line : \"%s\". ",
                    line.c_str() ) ;
            isNumaInUse = TRUE ;
            goto done ;
         }

         if( line.find( interleave, iLoc ) != iLoc )
         {
            PD_LOG( PDWARNING, "You are running on a NUMA machine."
                    " NUMA may cause performance problems. "
                    "SequoiaDB suggest you set NUMA closed in BIOS" ) ;
            isNumaInUse = TRUE ;
            goto done ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Open /proc/self/numa_maps failed" ) ;
         goto done ;
      }

   done:
      if ( f.is_open() )
      {
         f.close() ;
      }
      return isNumaInUse ;
   }

   INT64 utilCheckVmParamVal( const CHAR *pathPtr,
                              const INT64 &idealValue )
   {
      BOOLEAN isVmParamOK = TRUE ;
      INT64 val ;
      if( SDB_OK == ossAccess( pathPtr ) )
      {
         std::fstream f( pathPtr, std::ios_base::in ) ;
         if( ! f.is_open())
         {
            PD_LOG( PDWARNING, "Open %s failed", pathPtr ) ;
            isVmParamOK = FALSE ;
            goto done ;
         }

         f >> val ;

         if( val != idealValue )
         {
            PD_LOG( PDWARNING, "%s is %d, SequoiaDB suggest setting it to %d. "
                    "Edit /etc/sysctl.conf to set it. ",
                    pathPtr, val, idealValue ) ;
            isVmParamOK = FALSE ;
            f.close();
            goto done ;
         }
         f.close();
      }
      else
      {
         PD_LOG( PDWARNING, "utilCheckVmParamVal() : %s not exists "
                 "or can not read. ", pathPtr ) ;
         isVmParamOK = FALSE ;
         goto done ;
      }

   done:
      return isVmParamOK ;
   }

   static INT64 _utilGetSysMemTotalVolKB()
   {
      INT64 memTotalVolKB = 0 ;
      std::fstream f( "/proc/meminfo", std::ios_base::in ) ;
      string::size_type beginLoc = 0 ;
      string::size_type endLoc   = 0 ;
      const CHAR space           = ' ' ;
      string volStr ;
      string line ;
      if( !f.is_open() )
      {
         PD_LOG( PDWARNING, "Open /proc/meminfo failed. " );
         goto done;
      }

      std::getline( f, line ) ;
      if( f.fail() )
      {
         PD_LOG( PDWARNING, "Can not parse /proc/meminfo. " ) ;
      }
      else
      {
         beginLoc = line.find_first_of( space ) + 1 ;
         while( line[beginLoc] == space )
         {
            ++beginLoc ;
         }
         endLoc        = line.find_first_of( space, beginLoc ) ;
         volStr        = line.substr( beginLoc, endLoc-beginLoc ) ;
         memTotalVolKB = ossAtoll( volStr.c_str() ) ;
      }

   done:
      if ( f.is_open() )
      {
         f.close() ;
      }
      return memTotalVolKB ;
   }

   BOOLEAN utilCheckVmStatus()
   {//return TRUE means all of the linux kernel vm parameters are best
    //for SequoiaDB ;
      vector<CHAR *> vmPathVec( 8 ) ;
      vector<INT64> vmIdealValVec( 8 ) ;

      vmPathVec[0]     = "/proc/sys/vm/swappiness" ;
      vmIdealValVec[0] = 0 ;

      vmPathVec[1]     = "/proc/sys/vm/dirty_ratio" ;
      vmIdealValVec[1] = 100 ;

      vmPathVec[2]     = "/proc/sys/vm/dirty_background_ratio" ;
      vmIdealValVec[2] = 40 ;

      vmPathVec[3]     = "/proc/sys/vm/dirty_expire_centisecs" ;
      vmIdealValVec[3] = 3000 ;

      vmPathVec[4]     = "/proc/sys/vm/vfs_cache_pressure" ;
      vmIdealValVec[4] = 200 ;

      vmPathVec[5]     = "/proc/sys/vm/min_free_kbytes" ;
      vmIdealValVec[5] = ( _utilGetSysMemTotalVolKB() / 100 )
                         * UTIL_VM_MINFREE_KBYTES_PERCENTAGE ;

      vmPathVec[6]     = "/proc/sys/vm/overcommit_memory" ;
      vmIdealValVec[6] = 2 ;

      vmPathVec[7]     = "/proc/sys/vm/overcommit_ratio" ;
      vmIdealValVec[7] = 85 ;

      UINT32 i         = 0 ;
      BOOLEAN goodStat = TRUE ;
      for( ; i < vmPathVec.size() ; i++ )
      {
         //walk through all the Parameters, and log the unmatch Parameter.
         if( vmIdealValVec[i] != utilCheckVmParamVal( vmPathVec[i],
                                                      vmIdealValVec[i] ) )
         {
            goodStat = FALSE ;
         }
      }

      return goodStat ;
   }

   static BOOLEAN _isIdealValue( const string &result,
                                 const string &idealValue,
                                 const string &seperator1,
                                 const string &seperator2,
                                 const string &path )
   {
      BOOLEAN isIdeal = TRUE ;
      string opMode ;
      string::size_type beginLoc = result.find( seperator1 ) ;
      string::size_type endLoc   = result.find( seperator2 ) ;
      if( string::npos == beginLoc || string::npos == endLoc ||
          beginLoc >= endLoc )
      {
         PD_LOG( PDWARNING, "Can not parse \"%s\" from %s. ",
                 result.c_str(), path.c_str() ) ;

         goto error ;
      }

      opMode = result.substr( beginLoc+1, endLoc-beginLoc-1 ) ;
      if( opMode.empty() )
      {
         PD_LOG( PDWARNING, "Invalid mode in \"%s\" from %s",
            result.c_str(), path.c_str() ) ;

         goto error ;
      }

      if( idealValue != opMode )
      {
         PD_LOG( PDWARNING, "Your system environment value in %s is %s, "
                  "but SequoiaDb suggest setting it to %s. ", path.c_str(),
                  opMode.c_str(), idealValue.c_str() ) ;

         goto error ;
      }

   done:
      return isIdeal ;
   error:
      isIdeal = FALSE ;
      goto done ;
   }

   static BOOLEAN _thpCheck( const string &path )
   {
      BOOLEAN isThpClosed = TRUE ;
      string seperator1 = "[", seperator2 = "]" ;
      string idealValue = "never" ;
      string line ;
      std::ifstream ifs( path.c_str() ) ;

      if( SDB_OK != ossAccess( path.c_str() ) )
      {
         PD_LOG( PDWARNING, "%s not exist or can not open. ", path.c_str() ) ;
         goto error ;
      }

      if( !ifs )
      {
         PD_LOG( PDWARNING, "%s can not open. ", path.c_str() ) ;
         goto error ;
      }

      if( !std::getline( ifs, line ) )
      {
         PD_LOG( PDWARNING, "Failed to read line from %s. ", path.c_str() ) ;
         goto error ;
      }

      if( !_isIdealValue( line, idealValue, seperator1, seperator2, path ) )
      {
         goto error ;
      }

   done:
      ifs.close() ;
      return isThpClosed ;
   error:
      isThpClosed = FALSE ;
      goto done ;
   }

   BOOLEAN utilCheckThpStatus() //thp : transparent_hugepage
   {
      BOOLEAN isThpClosed  = TRUE ;
      const string thpPath = "/sys/kernel/mm/transparent_hugepage" ;
      const string thpEnabledPath =
                                 "/sys/kernel/mm/transparent_hugepage/enabled" ;
      const string thpDefragPath  =
                                 "/sys/kernel/mm/transparent_hugepage/defrag" ;

      if( SDB_OK != ossAccess( thpPath.c_str() ) )
      {
         PD_LOG( PDWARNING, "%s not exist or can not open. ",
                 thpPath.c_str() ) ;
         goto error ;
      }

      if( !_thpCheck( thpEnabledPath ) )
      {
         goto error ;
      }

      if( !_thpCheck( thpDefragPath ) )
      {
         goto error ;
      }

   done:
      return isThpClosed ;
   error:
      isThpClosed = FALSE ;
      goto done ;
   }


   BOOLEAN utilCheckEnv()
   {//return TRUE if all the envrionment settings are rightly set for SequoiaDB
      BOOLEAN goodEnv = TRUE ;

      goodEnv &= utilCheckIs64BitSys() ;
      goodEnv &= !utilCheckIsOpenVZ() ;
      goodEnv &= !utilCheckNumaStatus() ;
      goodEnv &= utilCheckVmStatus() ;
      goodEnv &= utilCheckThpStatus() ;

      return goodEnv ;
   }


}



