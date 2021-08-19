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

   Source File Name = qgmPlCommand.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLCOMMAND_HPP_
#define QGMPLCOMMAND_HPP_

#include "qgmPlan.hpp"
#include "msg.h"
#include "msgDef.h"
#include "utilResult.hpp"

namespace engine
{
   class _qgmPlCommand : public _qgmPlan
   {
   public:
      _qgmPlCommand( INT32 type,
                     const qgmDbAttr &fullName,
                     const qgmField &indexName,
                     const qgmOPFieldVec &indexColumns,
                     const BSONObj &partition,
                     BOOLEAN uniqIndex ) ;

      virtual ~_qgmPlCommand() ;

   public:
      virtual void close() ;

      virtual string toString() const ;

      virtual BOOLEAN needRollback() const ;

      virtual void    buildRetInfo( BSONObjBuilder &builder ) const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next ) ;

      INT32 _executeOnData( _pmdEDUCB *eduCB ) ;

      INT32 _executeOnCoord( _pmdEDUCB *eduCB ) ;

      void _killContext() ;

   private:
      INT32 _commandType ;
      INT64 _contextID ;
      qgmDbAttr _fullName ;
      qgmField _indexName ;
      qgmOPFieldVec _indexColumns ;
      BSONObj _partition ;
      BOOLEAN _uniqIndex ;

      utilWriteResult   _wrResult ;

   } ;

   typedef class _qgmPlCommand qgmPlCommand ;
}

#endif

