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

   Source File Name = rtnAlterJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERJOB_HPP_
#define RTN_ALTERJOB_HPP_

#include "rtnAlterDef.hpp"
#include "rtnAlterFuncList.hpp"

namespace engine
{
   class _rtnAlterJob : public SDBObject
   {
   public:
      _rtnAlterJob() ;
      ~_rtnAlterJob() ;

   public:
      OSS_INLINE const _rtnAlterOptions &getOptions() const
      {
         return _options ;
      }

      OSS_INLINE RTN_ALTER_TYPE getType() const
      {
         return _type ;
      }

      OSS_INLINE const CHAR *getName() const
      {
         return _name ;
      }

      OSS_INLINE INT32 getVersion() const
      {
         return _v ;
      }

      OSS_INLINE const bson::BSONObj &getOptionObj() const
      {
         return _optionsObj ;
      }

      OSS_INLINE const bson::BSONObj &getJobObj() const
      {
         return _obj ;
      }

      OSS_INLINE const bson::BSONObj &getTasks() const
      {
         return _tasks ;
      }

      OSS_INLINE BOOLEAN isEmpty() const
      {
         return _obj.isEmpty() ;
      }

   public:
      INT32 init( const bson::BSONObj &obj ) ;

      void clear() ;

   private:
      RTN_ALTER_TYPE _getObjType( const CHAR *name ) const ;

      void _extractOptions( const bson::BSONObj &obj ) ;

      INT32 _extractTasks( const bson::BSONElement &tasks,
                           bson::BSONArrayBuilder &builder ) ;

   private:
      _rtnAlterOptions _options ;
      RTN_ALTER_TYPE _type ;
      const CHAR *_name ;
      INT32 _v ;
      bson::BSONObj _obj ;
      bson::BSONObj _tasks ;
      bson::BSONObj _optionsObj ;
      _rtnAlterFuncList _fl ;
   } ;
}

#endif

