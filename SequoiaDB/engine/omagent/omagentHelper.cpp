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

   Source File Name = omagentHelper.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentHelper.hpp"
#include "omagentUtil.hpp"
#include "pmd.hpp"

namespace engine
{

   /*
      omagent control func
   */
   BOOLEAN omaIsCommand ( const CHAR *name )
   {
      if ( name && '$' == name[0] )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 omaParseCommand ( const CHAR *name, _omaCommand **ppCommand )
   {
      INT32 rc = SDB_INVALIDARG ;
      if ( ppCommand && omaIsCommand ( name ) )
      {
         *ppCommand = getOmaCmdBuilder()->create ( &name[1] ) ;
         if ( *ppCommand )
         {
            rc = SDB_OK ;
         }
      }
      return rc ;
   }

   INT32 omaInitCommand ( _omaCommand *pCommand ,INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn, const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff, const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      if ( !pCommand )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         rc = pCommand->init ( pMatcherBuff ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Omagent failed to init omsvc's command, rc = %d", rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "omagent init command[%s] exception[%s]",
                  pCommand->name(), e.what() ) ;
         rc = SDB_INVALIDARG ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omaRunCommand ( _omaCommand *pCommand, BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      if ( !pCommand )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !pmdGetKRCB()->isBusinessOK() &&
                pCommand->needCheckBusiness() )
      {
         rc = SDBCM_NODE_IS_IN_RESTORING ;
         goto error ;
      }
      try
      {
         rc = pCommand->doit ( result ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "omagent run command[%s] failed[rc=%d]",
                    pCommand->name(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "omagent run command[%s] exception[%s]",
                  pCommand->name(), e.what() ) ;
         rc = SDB_INVALIDARG ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 omaReleaseCommand ( _omaCommand **ppCommand )
   {
     INT32 rc = SDB_OK ;
      if ( ppCommand && *ppCommand )
      {
         getOmaCmdBuilder()->release( *ppCommand ) ;
         *ppCommand = NULL ;
      }
      return rc ;
   }

   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned,
                                vector<BSONObj> *objList )
   {
      SDB_ASSERT ( ppBuffer && bufferSize && objList, "Invalid input" ) ;
      INT32 rc             = SDB_OK ;
      INT32 packetLength = ossRoundUpToMultipleX ( 0, 4 ) ;
      if ( packetLength < 0 )
      {
         PD_LOG ( PDERROR, "Packet size overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      for ( UINT32 i = 0; i < objList->size(); i ++ )
      {
         rc = checkBuffer ( ppBuffer, bufferSize, packetLength ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to check buffer" ) ;
            goto error ;
         }
         ossMemcpy ( &((*ppBuffer)[packetLength]), (*objList)[i].objdata(),
                                                   (*objList)[i].objsize() ) ;
         packetLength += ossRoundUpToMultipleX ( (*objList)[i].objsize(), 4 ) ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned, const BSONObj *bsonobj )
   {
      SDB_ASSERT ( ppBuffer && bufferSize, "Invalid input" ) ;
      INT32 rc           = SDB_OK ;
      INT32 packetLength = ossRoundUpToMultipleX ( bsonobj->objsize(), 4 ) ;
      if ( packetLength < 0 )
      {
         PD_LOG ( PDERROR, "Packet size overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = checkBuffer ( ppBuffer, bufferSize, packetLength ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to check buffer, rc: %d", rc ) ;
         goto error ;
      }
      if ( numReturned != 0 )
      {
         ossMemcpy ( *ppBuffer, bsonobj->objdata(), bsonobj->objsize() ) ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

}
