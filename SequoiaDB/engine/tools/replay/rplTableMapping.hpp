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

   Source File Name = rplTableMapping.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_TABLE_MAPPING_HPP_
#define REPLAY_TABLE_MAPPING_HPP_

#include "oss.hpp"
#include "rplOutputter.hpp"
#include "rplField.hpp"
#include <string>
#include <map>

using namespace std ;
using namespace engine ;

namespace replay
{
   const INT32 MAX_CS_NAME_LEN = 255 ;
   const INT32 MAX_CL_NAME_LEN = 255 ;

   /*{ tables:
       [
         {
           source: "cs.cl",
           target: "dbname.tablename",
           fields:
           [
             {
               source: "field1",
               target: "column1",
               targetType: 0   // see EN_FieldType
             }
           ]
         }
       ]
     }
   */

   typedef vector< rplField* > FIELD_VECTOR ;
   class rplFieldMapping : public SDBObject
   {
   public:
      rplFieldMapping() ;
      ~rplFieldMapping() ;

   public:
      INT32 init( const CHAR * sourceFullName, const CHAR *tagetFullName ) ;
      INT32 addField( const BSONObj &field ) ;

      const CHAR* getTargetDBName() const ;

      const CHAR* getTargetTableName() const ;

      FIELD_VECTOR *getFieldVector() ;

   private:
      FIELD_VECTOR _fieldVector ;

      CHAR _sourceCSName[ MAX_CS_NAME_LEN + 1 ] ;
      CHAR _sourceCLName[ MAX_CL_NAME_LEN + 1 ] ;
      CHAR  _targetDBName[ MAX_CS_NAME_LEN + 1 ] ;
      CHAR  _targetTableName[ MAX_CL_NAME_LEN + 1 ] ;
   } ;

   class rplTableMapping : public SDBObject
   {
   public:
      rplTableMapping() ;
      ~rplTableMapping() ;

   public:
      INT32 init( const BSONObj &conf ) ;
      rplFieldMapping *getFieldMapping( const CHAR *clFullName ) ;
      void clear() ;

   private:
      INT32 _init( const BSONObj &conf ) ;
      INT32 _parseTable( const BSONObj &table ) ;
      INT32 _parseFields( const BSONObj fields, rplFieldMapping *fieldMapping ) ;

   private:
      // sourceCLFullName <-> rplFieldMapping
      typedef map< string, rplFieldMapping* > TABLE_MAP ;
      TABLE_MAP _tableMap ;
   } ;
}

#endif  /* REPLAY_TABLE_MAPPING_HPP_ */


