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

   Source File Name = rtnAlterJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft
          01/03/2018  HGM Improvement

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERJOB_HPP_
#define RTN_ALTERJOB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilMap.hpp"
#include "rtnAlterTask.hpp"
#include "rtnAlter.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   typedef _utilStringMap< rtnAlterTaskSchema, RTN_ALTER_MAX_ACTION > RTN_ALTER_TASK_MAP ;
   typedef ossPoolList< rtnAlterTask * > RTN_ALTER_TASK_LIST ;

   /*
      _rtnAlterTaskMap define
    */
   class _rtnAlterTaskMap : public SDBObject
   {
      public :
         _rtnAlterTaskMap () ;
         virtual ~_rtnAlterTaskMap () ;

         const rtnAlterTaskSchema & getSetAttrTask ( RTN_ALTER_OBJECT_TYPE objectType ) const ;

         const rtnAlterTaskSchema & getAlterTask ( RTN_ALTER_OBJECT_TYPE objectType,
                                                   const CHAR * name ) const ;

      protected :
         void _initialize () ;
         void _registerTask ( const CHAR * name,
                              RTN_ALTER_OBJECT_TYPE objectType,
                              RTN_ALTER_ACTION_TYPE actionType,
                              UINT32 flags = RTN_ALTER_TASK_FLAG_EMPTY ) ;
         RTN_ALTER_TASK_MAP * _getTaskMap ( RTN_ALTER_OBJECT_TYPE objectType ) ;
         const RTN_ALTER_TASK_MAP * _getTaskMap ( RTN_ALTER_OBJECT_TYPE objectType ) const ;

      protected :
         RTN_ALTER_TASK_MAP   _collectionTaskMap ;
         RTN_ALTER_TASK_MAP   _collectionSpaceTaskMap ;
         RTN_ALTER_TASK_MAP   _domainTaskMap ;
   } ;

   typedef class _rtnAlterTaskMap rtnAlterTaskMap ;

   const rtnAlterTaskMap & rtnGetAlterTaskMap () ;
   const rtnAlterTaskSchema & rtnGetSetAttrTask ( RTN_ALTER_OBJECT_TYPE objectType ) ;
   const rtnAlterTaskSchema & rtnGetAlterTask ( RTN_ALTER_OBJECT_TYPE objectType,
                                                const CHAR * name ) ;

   /*
      _rtnAlterJob define
    */
   class _rtnAlterJob : public SDBObject
   {
      public :
         _rtnAlterJob () ;
         virtual ~_rtnAlterJob () ;

      public :
         INT32 initialize ( const CHAR * objectName,
                            RTN_ALTER_OBJECT_TYPE objectType,
                            const bson::BSONObj & jobObject ) ;

         OSS_INLINE RTN_ALTER_OBJECT_TYPE getObjectType () const
         {
            return _objectType ;
         }

         OSS_INLINE const _rtnAlterOptions * getOptions () const
         {
            return &_options ;
         }

         OSS_INLINE const _rtnAlterInfo * getAlterInfo () const
         {
            return &_alterInfo ;
         }

         OSS_INLINE const RTN_ALTER_TASK_LIST & getAlterTasks () const
         {
            return _alterTasks ;
         }

         OSS_INLINE const CHAR *getObjectName () const
         {
            return _objectName ;
         }

         OSS_INLINE INT32 getVersion() const
         {
            return _version ;
         }

         OSS_INLINE const bson::BSONObj & getJobObject () const
         {
            return _jobObject ;
         }

         OSS_INLINE BOOLEAN isEmpty() const
         {
            return _alterTasks.empty() ;
         }

         OSS_INLINE INT32 getParseRC () const
         {
            return _parseRC ;
         }

      protected :
         static RTN_ALTER_OBJECT_TYPE _getObjectType ( const CHAR * name ) ;

         void _extractOptions ( const bson::BSONObj & obj ) ;

         INT32 _extractTask ( const bson::BSONObj & taskObject ) ;

         INT32 _createTask ( const rtnAlterTaskSchema & taskSchema,
                             const bson::BSONObj & arguments ) ;

         INT32 _extractSetAttrTask ( const bson::BSONObj & argument ) ;

         INT32 _extractTasks ( const bson::BSONElement & taskObjects ) ;

         void _clearTasks () ;

         void _clearJob () ;

      protected :
         RTN_ALTER_OBJECT_TYPE   _objectType ;
         bson::BSONObj           _jobObject ;
         rtnAlterOptions         _options ;
         RTN_ALTER_TASK_LIST     _alterTasks ;
         const CHAR *            _objectName ;
         INT32                   _version ;
         INT32                   _parseRC ;
         rtnAlterInfo            _alterInfo ;
   } ;

   typedef class _rtnAlterJob rtnAlterJob ;

   /*
      _rtnAlterJobHolder define
    */
   class _rtnAlterJobHolder ;
   typedef class _rtnAlterJobHolder rtnAlterJobHolder ;

   class _rtnAlterJobHolder
   {
      public :
         _rtnAlterJobHolder () ;
         virtual ~_rtnAlterJobHolder () ;

         INT32 createAlterJob () ;
         void deleteAlterJob () ;
         void setAlterJob ( rtnAlterJobHolder & holder, BOOLEAN getOwned ) ;

         OSS_INLINE rtnAlterJob * getAlterJob ()
         {
            return _alterJob ;
         }

         OSS_INLINE const rtnAlterJob * getAlterJob () const
         {
            return _alterJob ;
         }

      protected :
         BOOLEAN        _ownedJob ;
         rtnAlterJob *  _alterJob ;
   } ;

}

#endif // RTN_ALTERJOB_HPP_
