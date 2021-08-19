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

   Source File Name = coordAuthOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordAuthOperator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAuthOperator implement
   */
   _coordAuthOperator::_coordAuthOperator()
   {
      const static string s_name( "Auth" ) ;
      setName( s_name ) ;
   }

   _coordAuthOperator::~_coordAuthOperator()
   {
   }

   BOOLEAN _coordAuthOperator::isReadOnly() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHOPR_EXEC, "_coordAuthOperator::execute" )
   INT32 _coordAuthOperator::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHOPR_EXEC ) ;

      rc = forward( pMsg, cb, TRUE, contextID, NULL, NULL, NULL, buf ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_AUTHOPR_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordAuthOperator::_onSucReply( const MsgOpReply *pReply )
   {
      if ( pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) )
      {
         try
         {
            BSONObj obj( ( const CHAR* )pReply + sizeof( MsgOpReply ) ) ;
            BSONElement e = obj.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object == e.type() )
            {
               updateSessionByOptions( e.embeddedObject() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
            /// ignore
         }
      }
   }

}

