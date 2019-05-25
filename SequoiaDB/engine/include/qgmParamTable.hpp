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

   Source File Name = qgmParamTable.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPARAMTABLE_HPP_
#define QGMPARAMTABLE_HPP_

#include "qgmDef.hpp"
#include "utilMap.hpp"
#include <vector>

using namespace bson ;

namespace engine
{
   struct _qgmBsonPair
   {
      BSONObj obj ;
      BSONElement ele ;
   } ;

   typedef std::list<_qgmBsonPair>                 QGM_CONST_TABLE ;
   typedef std::map<qgmDbAttr, BSONElement >       QGM_VAR_TABLE ;

   class _qgmParamTable : public SDBObject
   {
   public:
      _qgmParamTable() ;
      virtual ~_qgmParamTable() ;

   public:
      INT32 addConst( const qgmOpField &value,
                      const BSONElement *&out ) ;

      INT32 addConst( const BSONObj &obj,
                      const BSONElement *&out ) ;

      INT32 addVar( const qgmDbAttr &key,
                    const BSONElement *&out,
                    BOOLEAN *pExisted = NULL ) ;

      INT32 setVar( const varItem &item,
                    const BSONObj &obj ) ;

      OSS_INLINE void removeVar( const qgmDbAttr &key )
      {
         _var.erase( key ) ;
         return ;
      }

      OSS_INLINE void clearVar()
      {
         _var.clear() ;
         return ;
      }

      OSS_INLINE void clearConst()
      {
         _const.clear() ;
         return ;
      }

      OSS_INLINE void clear()
      {
         _var.clear() ;
         _const.clear() ;
         return ;
      }

   private:
      QGM_CONST_TABLE _const ;
      QGM_VAR_TABLE _var ;
   } ;
   typedef class _qgmParamTable qgmParamTable ;
}

#endif

