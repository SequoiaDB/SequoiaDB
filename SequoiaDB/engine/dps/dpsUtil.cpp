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

#define DPS_RECORD_FLAGS_NON_BUSINESSOP            "NonBusinessOP"
#define DPS_STATUS_SEPARATOR                       " | "

   dpsLogConfig &dpsGetGlobalLogConfig()
   {
      static dpsLogConfig g_logConfig ;
      return g_logConfig ;
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

   INT32 dpsGetTransIDFromString( const CHAR *pStr, DPS_TRANS_ID &transID )
   {
      INT32 rc = SDB_OK ;
      if ( '\0' != pStr[0] && '\0' != pStr[1] )
      {
         if ( '0' == pStr[0] && ( 'x' == pStr[1] || 'X' == pStr[1] ) )
         {
            //16 base string
            UINT32 nodeID = 0 ;
            ossSscanf( pStr, "0x%04x%010llx", &nodeID, &transID ) ;
            transID = (UINT64)nodeID << DPS_TRANSID_NODEID_POW | transID ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;

   done:
      return rc ;
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
      try
      {
         return dpsTransIDToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
      }
      catch( std::exception &e )
      {
         try
         {
            return e.what() ;
         }
         catch (...)
         {
            return "Out-of-memory" ;
         }
      }
   }

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
      try
      {
         return dpsTransIDAttrToString( transID, tmpStr, DPS_TRANS_STR_LEN ) ;
      }
      catch( std::exception &e )
      {
         try
         {
            return e.what() ;
         }
         catch (...)
         {
            return "Out-of-memory" ;
         }
      }
   }

   void dpsAppendFlagString( CHAR * pBuffer, INT32 bufSize,
                                 const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DPS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   void dpsFlags2String( UINT16 flags, CHAR * pBuffer, INT32 bufSize )
   {
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;

      // business operation
      if ( OSS_BIT_TEST( flags, DPS_FLG_NON_BS_OP ) )
      {
         dpsAppendFlagString( pBuffer, bufSize, DPS_RECORD_FLAGS_NON_BUSINESSOP ) ;
      }
   }

   DPS_TRANS_ID dpsTransIDDowngrade( const dpsTransID_v1 &transID )
   {
      DPS_TRANS_ID resultID = DPS_INVALID_TRANS_ID ;
      if ( DPS_INVALID_TRANS_ID != transID._sn )
      {
         ( ( (DPS_TRANS_ID)transID._nodeID ) << 48 ) ;
         resultID |= ( transID._sn & DPS_TRANSID_SN_BIT ) ;
         resultID |= ( ( transID._sn & 0xF000000000000000 ) >> 20 ) ;
         if ( transID._sn & 0x0100000000000000 )
         {
            PD_LOG( PDWARNING, "Will lose bits of global timestamp [%llx]",
                    transID._sn ) ;
         }
      }
      return resultID ;
   }

}

