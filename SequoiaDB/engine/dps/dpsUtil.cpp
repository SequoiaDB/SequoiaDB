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

   Source File Name = dpsTransVersionCtrl.cpp

   Descriptive Name = dps transaction version control

   When/how to use: this program may be used on binary and text-formatted
   versions of Data Protection component. This file contains functions for
   transaction isolation control through version control implmenetation.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2019  Linyoub  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsUtil.hpp"
#include "dpsTransDef.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

namespace engine
{
   static BOOLEAN g_TimeonFlag = FALSE ;

   void dpsSetTimeonFlag( BOOLEAN flag )
   {
      g_TimeonFlag = flag ;
   }

   BOOLEAN dpsGetTimeonFlag()
   {
      return g_TimeonFlag ;
   }

   const CHAR* dpsTransStatusToString( INT32 status )
   {
      const CHAR *pStr = "Unknown" ;
      switch ( status )
      {
         case DPS_TRANS_DOING :
            pStr = "Doing" ;
            break ;
         case DPS_TRANS_WAIT_COMMIT :
            pStr = "WaitCommit" ;
            break ;
         case DPS_TRANS_COMMIT :
            pStr = "Commited" ;
            break ;
         case DPS_TRANS_ROLLBACK :
            pStr = "Rollbacked" ;
            break ;
         case DPS_TRANS_DOING_INTERRUPT :
            pStr = "DoingInterrupted" ;
            break ;
         default :
            break ;
      }
      return pStr ;
   }

   const CHAR* dpsTransIDToString( const DPS_TRANS_ID &transID,
                                   CHAR *pBuff,
                                   UINT32 bufSize )
   {
      SDB_ASSERT( pBuff && bufSize > 0, "Invalid input" ) ;

      ossSnprintf( pBuff, bufSize, "0x%04x%010llx",
                   DPS_TRANS_GET_NODEID( transID ),
                   DPS_TRANS_GET_SN( transID ) ) ;

      return pBuff ;
   }

   ossPoolString dpsTransIDToString( const DPS_TRANS_ID &transID )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      return dpsTransIDToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
   }

   #define DPS_STATUS_SEPARATOR                       " | "

   static void _dpsAppendFlagString( CHAR *pBuffer,
                                     UINT32 bufSize,
                                     const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DPS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   const CHAR* dpsTransIDAttrToString( const DPS_TRANS_ID &transID,
                                       CHAR *pBuff,
                                       UINT32 bufSize )
   {
      SDB_ASSERT( pBuff && bufSize > 0, "Invalid input" ) ;

      if ( DPS_TRANS_IS_FIRSTOP( transID ) )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Start" ) ;
      }
      if ( DPS_TRANS_IS_AUTOCOMMIT( transID ) )
      {
         _dpsAppendFlagString( pBuff, bufSize, "AutoCommit" ) ;
      }
      if ( DPS_TRANS_IS_ROLLBACK( transID ) )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Rollback" ) ;
      }
      if ( DPS_TRANS_IS_RBPENDING( transID ) )
      {
         _dpsAppendFlagString( pBuff, bufSize, "Pending" ) ;
      }

      return pBuff ;
   }

   ossPoolString dpsTransIDAttrToString( const DPS_TRANS_ID &transID )
   {
      CHAR tmpStr[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
      return dpsTransIDAttrToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
   }

}

