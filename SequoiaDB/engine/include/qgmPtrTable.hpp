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

   Source File Name = qgmPtrTable.hpp

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

#ifndef QGMPTRTABLE_HPP_
#define QGMPTRTABLE_HPP_

#include "qgmDef.hpp"
#include "ossMemPool.hpp"
#include "ossMem.hpp"

namespace engine
{
   /*
      qgm_char_cmp define
   */
   struct qgm_char_cmp
   {
      bool operator()( const CHAR *a, const CHAR *b )
      {
         return ossStrcmp( a, b ) < 0 ;
      }
   } ;

   typedef ossPoolSet<qgmField>                             PTR_TABLE ;
   typedef ossPoolMap<const CHAR*, UINT32, qgm_char_cmp>    STR_TABLE ;
#if defined (_WINDOWS)
   typedef STR_TABLE::iterator                              STR_TABLE_IT ;
#else
   typedef ossPoolMap<const CHAR*, UINT32>::iterator        STR_TABLE_IT ;
#endif // _WINDOWS

   /*
      _qgmPtrTable define
   */
   class _qgmPtrTable : public _utilPooledObject
   {
   public:
      _qgmPtrTable() ;
      virtual ~_qgmPtrTable() ;

   public:
      INT32 getField( const SQL_CON_ITR &itr,
                      qgmField &field ) ;

      INT32 getField( const CHAR *begin, UINT32 size,
                      qgmField &field ) ;

      INT32 getOwnField( const CHAR *begin, qgmField &field ) ;
      INT32 getOwnField( const CHAR *begin, UINT32 size,
                         qgmField &field ) ;

      INT32 getAttr( const SQL_CON_ITR &itr,
                     qgmDbAttr &attr ) ;

      INT32 getAttr( const CHAR *begin, UINT32 size,
                     qgmDbAttr &attr ) ;

      INT32 getOwnAttr( const CHAR *begin, UINT32 size,
                        qgmDbAttr &attr ) ;

      INT32 getUniqueFieldAlias( qgmField &field ) ;
      INT32 getUniqueTableAlias( qgmField &field ) ;

      qgmField    getField( const qgmField &sub1,
                            const qgmField &sub2 ) ;
      const CHAR* getOwnedString( const CHAR *str ) ;

   private:
      PTR_TABLE _table ;
      STR_TABLE _stringTable ;
      UINT32    _uniqueFieldID ;
      UINT32    _uniqueTableID ;

   } ;

   typedef class _qgmPtrTable qgmPtrTable ;
}

#endif // QGMPTRTABLE_HPP_

