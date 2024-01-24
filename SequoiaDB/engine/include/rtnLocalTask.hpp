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

   Source File Name = rtnLocalTask.hpp

   Descriptive Name = Local Task

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOCAL_TASK_HPP__
#define RTN_LOCAL_TASK_HPP__

#include "dms.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "utilRecycleItem.hpp"

using namespace bson ;

namespace engine
{

   /*
      Field Name define
   */
   #define RTN_LT_FIELD_FROM                       "From"
   #define RTN_LT_FIELD_TO                         "To"
   #define RTN_LT_FIELD_TIME                       "Time"

   /*
   Rename CS Format
   {
      _id: <id>,
      Type: 1,
      TypeDesp: "RENAMECS",
      From: "my",
      To: "test",
      Time: "2020-04-27-10:30:30.000000"
   }

   Rename CL Format
   {
      _id: <id>,
      Type: 2,
      TypeDesp: "RENAMECL",
      Form: "my.my",
      To: "my.test",
      Time: "2020-04-27-10:30:30.000000"
   }
   */
   /*
      _rtnLTRename define
   */
   class _rtnLTRename : public _rtnLocalTaskBase
   {
      public:
         _rtnLTRename() ;
         virtual ~_rtnLTRename() ;

         void           setInfo( const CHAR *from, const CHAR *to ) ;
         const CHAR*    getFrom() const ;
         const CHAR*    getTo() const ;

      public:
         virtual INT32     initFromBson( const BSONObj &obj ) ;
         virtual BOOLEAN   muteXOn ( const _rtnLocalTaskBase *pOther ) const ;

      protected:
         virtual INT32     _toBson( BSONObjBuilder &builder ) const ;

         BOOLEAN _isSameLevel( const _rtnLocalTaskBase *pOther ) const ;

      private:
         CHAR  _from[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR  _to[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR  _time[ OSS_TIMESTAMP_STRING_LEN + 1 ] ;

   } ;
   typedef _rtnLTRename rtnLTRename ;

   /*
      _rtnLTRenameCS define
   */
   class _rtnLTRenameCS : public _rtnLTRename
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;
      public:
         _rtnLTRenameCS() ;
         virtual ~_rtnLTRenameCS() ;

         virtual RTN_LOCAL_TASK_TYPE getTaskType() const ;
   } ;
   typedef _rtnLTRenameCS rtnLTRenameCS ;

   /*
      _rtnLTRenameCL define
   */
   class _rtnLTRenameCL : public _rtnLTRename
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;
      public:
         _rtnLTRenameCL() ;
         virtual ~_rtnLTRenameCL() ;

         virtual RTN_LOCAL_TASK_TYPE getTaskType() const ;
   } ;
   typedef _rtnLTRenameCL rtnLTRenameCL ;

   /*
      _rtnLTRecycleBase define
    */
   class _rtnLTRecycleBase : public _rtnLTRename
   {
   protected:
      typedef class _rtnLTRename _BASE ;

   public:
      _rtnLTRecycleBase() ;
      virtual ~_rtnLTRecycleBase() ;

      OSS_INLINE void setRecycleItem( const utilRecycleItem &recycleItem )
      {
         _recycleItem = recycleItem ;
      }

      OSS_INLINE const utilRecycleItem &getRecycleItem() const
      {
         return _recycleItem ;
      }

      void setInfo( const CHAR *from,
                    const CHAR *to,
                    const utilRecycleItem &item ) ;

      virtual INT32 initFromBson( const BSONObj &obj ) ;

   protected:
      virtual INT32 _toBson( BSONObjBuilder &builder ) const ;

   protected:
      utilRecycleItem _recycleItem ;
   } ;

   typedef class _rtnLTRecycleBase rtnLTRecycleBase ;

   /*
      _rtnLTRecycleCS define
    */
   class _rtnLTRecycleCS : public _rtnLTRecycleBase
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;

   public:
      _rtnLTRecycleCS() {}
      virtual ~_rtnLTRecycleCS() {}

      RTN_LOCAL_TASK_TYPE getTaskType() const
      {
         return RTN_LOCAL_TASK_RECYCLECS ;
      }
   } ;

   typedef class _rtnLTRecycleCS rtnLTRecycleCS ;

   /*
      _rtnLTRecycleCL define
    */
   class _rtnLTRecycleCL : public _rtnLTRecycleBase
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;

   public:
      _rtnLTRecycleCL() {}
      virtual ~_rtnLTRecycleCL() {}

      RTN_LOCAL_TASK_TYPE getTaskType() const
      {
         return RTN_LOCAL_TASK_RECYCLECL ;
      }
   } ;

   typedef class _rtnLTRecycleCL rtnLTRecycleCL ;

   /*
      _rtnLTReturnCS define
    */
   class _rtnLTReturnCS : public _rtnLTRecycleBase
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;

   public:
      _rtnLTReturnCS() {}
      virtual ~_rtnLTReturnCS() {}

      RTN_LOCAL_TASK_TYPE getTaskType() const
      {
         return RTN_LOCAL_TASK_RETURNCS ;
      }
   } ;

   typedef class _rtnLTReturnCS rtnLTReturnCS ;

   /*
      _rtnLTReturnCL define
    */
   class _rtnLTReturnCL : public _rtnLTRecycleBase
   {
      RTN_DECLARE_LT_AUTO_REGISTER() ;

   public:
      _rtnLTReturnCL() {}
      virtual ~_rtnLTReturnCL() {}

      RTN_LOCAL_TASK_TYPE getTaskType() const
      {
         return RTN_LOCAL_TASK_RETURNCL ;
      }
   } ;

   typedef class _rtnLTReturnCL rtnLTReturnCL ;

}

#endif //RTN_LOCAL_TASK_HPP__
