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

   Source File Name = rtnDataSet.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/06/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_BSONSET_HPP_
#define RTN_BSONSET_HPP_

#include "rtnContextBuff.hpp"
#include "rtnQueryOptions.hpp"
#include "msg.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_RTNCB ;

   class _rtnDataSet : public SDBObject
   {
   public:
      _rtnDataSet( const rtnQueryOptions &options, _pmdEDUCB *cb ) ;
      _rtnDataSet( SINT64 contextID, _pmdEDUCB *cb ) ;
      _rtnDataSet( MsgOpReply *pReply, _pmdEDUCB *cb,
                   BOOLEAN ownned = FALSE ) ;

      virtual ~_rtnDataSet() ;

   protected:
      void     clear() ;

      INT32    initByQuery( const rtnQueryOptions &options,
                            _pmdEDUCB *cb ) ;

      INT32    initByReply( MsgOpReply *pReply, _pmdEDUCB *cb,
                            BOOLEAN ownned = FALSE ) ;

   public:
      INT32 next( BSONObj &obj ) ;

   private:
      rtnContextBuf     _contextBuf ;
      SINT64            _contextID ;
      _pmdEDUCB         *_cb ;
      INT32             _lastErr ;
      _SDB_RTNCB        *_rtnCB ;
      CHAR              *_pBuff ;
      BOOLEAN           _ownned ;
   } ;
   typedef class _rtnDataSet rtnDataSet ;
}

#endif // RTN_BSONSET_HPP_

