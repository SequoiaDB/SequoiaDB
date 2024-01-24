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

   Source File Name = rtnAlterJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnAlterJob.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include "msgDef.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _rtnAlterTaskMap implement
    */
   _rtnAlterTaskMap::_rtnAlterTaskMap ()
   {
      _initialize() ;
   }

   _rtnAlterTaskMap::~_rtnAlterTaskMap ()
   {
   }

   const rtnAlterTaskSchema & _rtnAlterTaskMap::getSetAttrTask ( RTN_ALTER_OBJECT_TYPE objectType ) const
   {
      return getAlterTask( objectType, SDB_ALTER_ACTION_SET_ATTR ) ;
   }

   const rtnAlterTaskSchema & _rtnAlterTaskMap::getAlterTask ( RTN_ALTER_OBJECT_TYPE objectType,
                                                               const CHAR * name ) const
   {
      static rtnAlterTaskSchema invalidSchema ;

      const RTN_ALTER_TASK_MAP * taskMap = _getTaskMap( objectType ) ;

      if ( NULL != taskMap )
      {
         RTN_ALTER_TASK_MAP::const_iterator iter = taskMap->find( name ) ;
         if ( iter != taskMap->end() )
         {
            return iter->second ;
         }
      }

      return invalidSchema ;
   }

   void _rtnAlterTaskMap::_initialize ()
   {
      /// Collection
      /// Create ID Index
      _registerTask( SDB_ALTER_CL_CRT_ID_INDEX,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_CREATE_ID_INDEX,
                     ( RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      // Create AutoIncrement Field
      _registerTask( SDB_ALTER_CL_CRT_AUTOINC_FLD,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_CREATE_AUTOINC_FLD,
                     ( RTN_ALTER_TASK_FLAG_CONTEXTLOCK |
                       RTN_ALTER_TASK_FLAG_SEQUENCE |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      /// Drop ID Index
      _registerTask( SDB_ALTER_CL_DROP_ID_INDEX,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_DROP_ID_INDEX,
                     ( RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      /// Increase Version
      _registerTask( SDB_ALTER_CL_INC_VER,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_INC_VERSION ) ;

      /// Drop AutoIncrement Field
      _registerTask( SDB_ALTER_CL_DROP_AUTOINC_FLD,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_DROP_AUTOINC_FLD,
                     ( RTN_ALTER_TASK_FLAG_CONTEXTLOCK |
                       RTN_ALTER_TASK_FLAG_SEQUENCE |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      _registerTask( SDB_ALTER_CL_ENABLE_SHARDING,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_ENABLE_SHARDING,
                     ( RTN_ALTER_TASK_FLAG_ROLLBACK |
                       RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_3PHASE |
                       RTN_ALTER_TASK_FLAG_CONTEXTLOCK |
                       RTN_ALTER_TASK_FLAG_SHARDONLY ) ) ;

      _registerTask( SDB_ALTER_CL_DISABLE_SHARDING,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_DISABLE_SHARDING,
                     ( RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_3PHASE |
                       RTN_ALTER_TASK_FLAG_CONTEXTLOCK |
                       RTN_ALTER_TASK_FLAG_SHARDONLY ) ) ;

      _registerTask( SDB_ALTER_CL_ENABLE_COMPRESS,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_ENABLE_COMPRESS,
                     ( RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_3PHASE |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      _registerTask( SDB_ALTER_CL_DISABLE_COMPRESS,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_DISABLE_COMPRESS,
                     ( RTN_ALTER_TASK_FLAG_SHARDLOCK |
                       RTN_ALTER_TASK_FLAG_3PHASE |
                       RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ) ;

      _registerTask( SDB_ALTER_CL_SET_ATTR,
                     RTN_ALTER_COLLECTION,
                     RTN_ALTER_CL_SET_ATTRIBUTES ) ;

      /// Collection space
      _registerTask( SDB_ALTER_CS_SET_DOMAIN,
                     RTN_ALTER_COLLECTION_SPACE,
                     RTN_ALTER_CS_SET_DOMAIN,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_CS_REMOVE_DOMAIN,
                     RTN_ALTER_COLLECTION_SPACE,
                     RTN_ALTER_CS_REMOVE_DOMAIN,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_CS_ENABLE_CAPPED,
                     RTN_ALTER_COLLECTION_SPACE,
                     RTN_ALTER_CS_ENABLE_CAPPED,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_CS_DISABLE_CAPPED,
                     RTN_ALTER_COLLECTION_SPACE,
                     RTN_ALTER_CS_DISABLE_CAPPED,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_CS_SET_ATTR,
                     RTN_ALTER_COLLECTION_SPACE,
                     RTN_ALTER_CS_SET_ATTRIBUTES,
                     RTN_ALTER_TASK_FLAG_3PHASE ) ;

      /// Domain
      _registerTask( SDB_ALTER_DOMAIN_ADD_GROUPS,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_ADD_GROUPS,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_DOMAIN_SET_GROUPS,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_SET_GROUPS,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_DOMAIN_REMOVE_GROUPS,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_REMOVE_GROUPS,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_DOMAIN_SET_ACTIVE_LOCATION,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_DOMAIN_SET_LOCATION,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_SET_LOCATION,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;

      _registerTask( SDB_ALTER_DOMAIN_SET_ATTR,
                     RTN_ALTER_DOMAIN,
                     RTN_ALTER_DOMAIN_SET_ATTRIBUTES,
                     RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
   }

   void _rtnAlterTaskMap::_registerTask ( const CHAR * name,
                                          RTN_ALTER_OBJECT_TYPE objectType,
                                          RTN_ALTER_ACTION_TYPE actionType,
                                          UINT32 flags )
   {
      RTN_ALTER_TASK_MAP * taskMap = _getTaskMap( objectType ) ;
      if ( NULL != taskMap && taskMap->end() == taskMap->find( name ) )
      {
         rtnAlterTaskSchema task( name, objectType, actionType, flags ) ;
         if ( task.isValid() )
         {
            taskMap->insert(
               RTN_ALTER_TASK_MAP::value_type( task.getActionName(), task ) ) ;
         }
      }
   }

   RTN_ALTER_TASK_MAP * _rtnAlterTaskMap::_getTaskMap ( RTN_ALTER_OBJECT_TYPE objectType )
   {
      RTN_ALTER_TASK_MAP * taskMap = NULL ;

      switch ( objectType )
      {
         case RTN_ALTER_COLLECTION :
            taskMap = &_collectionTaskMap ;
            break ;
         case RTN_ALTER_COLLECTION_SPACE :
            taskMap = &_collectionSpaceTaskMap ;
            break ;
         case RTN_ALTER_DOMAIN :
            taskMap = &_domainTaskMap ;
            break ;
         default :
            taskMap = NULL ;
            break ;
      }

      return taskMap ;
   }

   const RTN_ALTER_TASK_MAP * _rtnAlterTaskMap::_getTaskMap ( RTN_ALTER_OBJECT_TYPE objectType ) const
   {
      const RTN_ALTER_TASK_MAP * taskMap = NULL ;

      switch ( objectType )
      {
         case RTN_ALTER_COLLECTION :
            taskMap = &_collectionTaskMap ;
            break ;
         case RTN_ALTER_COLLECTION_SPACE :
            taskMap = &_collectionSpaceTaskMap ;
            break ;
         case RTN_ALTER_DOMAIN :
            taskMap = &_domainTaskMap ;
            break ;
         default :
            taskMap = NULL ;
            break ;
      }

      return taskMap ;
   }

   static rtnAlterTaskMap s_alterTaskMap ;

   const rtnAlterTaskMap & rtnGetAlterTaskMap ()
   {
      return s_alterTaskMap ;
   }

   const rtnAlterTaskSchema & rtnGetSetAttrTask ( RTN_ALTER_OBJECT_TYPE objectType )
   {
      return s_alterTaskMap.getSetAttrTask( objectType ) ;
   }

   const rtnAlterTaskSchema & rtnGetAlterTask ( RTN_ALTER_OBJECT_TYPE objectType,
                                                const CHAR * name )
   {
      return s_alterTaskMap.getAlterTask( objectType, name ) ;
   }

   /*
      _rtnAlterJob implement
    */
   _rtnAlterJob::_rtnAlterJob ()
   : _objectType( RTN_ALTER_INVALID_OBJECT ),
     _objectName( NULL ),
     _version( 0 ),
     _parseRC( SDB_OK )
   {
   }

   _rtnAlterJob::~_rtnAlterJob ()
   {
      _clearTasks() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB_INITIALIZE, "_rtnAlterJob::initialize" )
   INT32 _rtnAlterJob::initialize ( const CHAR * objectName,
                                    RTN_ALTER_OBJECT_TYPE objectType,
                                    const bson::BSONObj & jobObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERJOB_INITIALIZE ) ;

      SDB_ASSERT( _jobObject.isEmpty(), "clear it first" ) ;

      _clearJob() ;

      try
      {
         BSONElement jobElement ;

         _jobObject = jobObject.getOwned() ;

         /// version
         jobElement = _jobObject.getField( FIELD_NAME_VERSION ) ;
         if ( jobElement.eoo() )
         {
            BSONObj options ;
            BSONObj autoIncOptions ;
            BSONObjBuilder autoIncBuilder ;
            BSONArrayBuilder autoIncArr ;

            _objectType = objectType ;

            if ( NULL != objectName )
            {
               _objectName = objectName ;
            }
            else if ( _jobObject.hasField( FIELD_NAME_NAME ) &&
                      String == _jobObject.getField( FIELD_NAME_NAME ).type() )
            {
               _objectName = _jobObject.getStringField( FIELD_NAME_NAME ) ;
            }

            PD_CHECK( NULL != _objectName, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get name" ) ;

            rc = rtnGetObjElement( _jobObject, FIELD_NAME_OPTIONS, options ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_OPTIONS, rc ) ;

            rc = _extractSetAttrTask( options ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create set attributes task, "
                         "rc: %d", rc ) ;
         }
         else
         {
            const CHAR * objectSchemaName = NULL ;

            PD_CHECK( jobElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type of field [%s] from request object [%s]",
                      FIELD_NAME_VERSION,
                      _jobObject.toString( FALSE, TRUE ).c_str() ) ;

            _version = jobElement.Number() ;
            PD_CHECK( SDB_ALTER_VERSION == _version,
                      SDB_INVALIDARG, error, PDERROR,
                      "invalid version: %d", _version ) ;

            /// type
            jobElement = _jobObject.getField( FIELD_NAME_ALTER_TYPE ) ;
            PD_CHECK( String == jobElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "invalid type of field [%s],request obj: %s",
                       FIELD_NAME_ALTER_TYPE,
                       _jobObject.toString( FALSE, TRUE ).c_str() ) ;

            objectSchemaName = jobElement.valuestrsafe() ;
            _objectType = _getObjectType( objectSchemaName ) ;
            PD_CHECK( RTN_ALTER_INVALID_OBJECT != _objectType,
                      SDB_INVALIDARG, error, PDERROR,
                      "Invalid alter object : %s", objectSchemaName ) ;

            /// object name
            jobElement = _jobObject.getField( FIELD_NAME_NAME ) ;
            PD_CHECK( String == jobElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "invalid type of field [%s],request obj: %s",
                      FIELD_NAME_NAME,
                      _jobObject.toString( FALSE, TRUE ).c_str() ) ;

            _objectName = jobElement.valuestrsafe() ;

            /// options is not necessary.
            jobElement = jobObject.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object == jobElement.type() )
            {
               _extractOptions( jobElement.embeddedObject() ) ;
            }

            /// alter list
            jobElement = _jobObject.getField( FIELD_NAME_ALTER ) ;
            PD_CHECK( jobElement.isABSONObj(), SDB_INVALIDARG, error, PDERROR,
                      "invalid type of field [%s],request obj: %s",
                      FIELD_NAME_ALTER,
                      _jobObject.toString( FALSE, TRUE ).c_str() ) ;

            rc = _extractTasks( jobElement ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to extract task list:%d", rc ) ;

            /// alter info
            jobElement = jobObject.getField( FIELD_NAME_ALTER_INFO ) ;
            if ( !jobElement.eoo() )
            {
               PD_CHECK( Object == jobElement.type(),
                         SDB_INVALIDARG, error, PDERROR,
                         "invalid type of field [%s],request obj: %s",
                         FIELD_NAME_ALTER_INFO,
                         _jobObject.toString( FALSE, TRUE ).c_str() ) ;

               rc = _alterInfo.init( jobElement.Obj() ) ;
               PD_RC_CHECK( rc, PDERROR, "failed to init alter info:%d", rc ) ;
            }
         }
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "unexpected error heppened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERJOB_INITIALIZE, rc ) ;
      return rc ;

   error :
      _clearJob() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__CLEARJOB, "_rtnAlterJob::_clearJob" )
   void _rtnAlterJob::_clearJob ()
   {
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__CLEARJOB ) ;

      _objectType = RTN_ALTER_INVALID_OBJECT ;
      _objectName = NULL ;
      _version = 0 ;
      _options.reset() ;
      _jobObject = BSONObj() ;
      _parseRC = SDB_OK ;

      _clearTasks() ;

      PD_TRACE_EXIT( SDB__RTNALTERJOB__CLEARJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTRACTOPTIONS, "_rtnAlterJob::_extractOptions" )
   void _rtnAlterJob::_extractOptions ( const BSONObj & obj )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTRACTOPTIONS ) ;

      BOOLEAN igore = obj.getBoolField( FIELD_NAME_IGNORE_EXCEPTION ) ;
      _options.setIgnoreException( igore ) ;

      PD_TRACE_EXIT( SDB__RTNALTERJOB__EXTRACTOPTIONS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__GETOBJTYPE, "_rtnAlterJob::_getObjectType" )
   RTN_ALTER_OBJECT_TYPE _rtnAlterJob::_getObjectType ( const CHAR * name )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__GETOBJTYPE ) ;

      RTN_ALTER_OBJECT_TYPE type = RTN_ALTER_INVALID_OBJECT ;

      if ( 0 == ossStrcmp( SDB_CATALOG_CL, name ) )
      {
         type = RTN_ALTER_COLLECTION ;
      }
      else if ( 0 == ossStrcmp( SDB_CATALOG_CS, name ) )
      {
         type = RTN_ALTER_COLLECTION_SPACE ;
      }
      else if ( 0 == ossStrcmp( SDB_CATALOG_DOMAIN, name ) )
      {
         type = RTN_ALTER_DOMAIN ;
      }
      else
      {
         PD_LOG( PDERROR, "invalid type: %s", name ) ;
      }

      PD_TRACE_EXIT( SDB__RTNALTERJOB__GETOBJTYPE ) ;

      return type ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTRACTTASK, "_rtnAlterJob::_extractTask" )
   INT32 _rtnAlterJob::_extractTask ( const BSONObj & taskObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTRACTTASK ) ;

      BSONObj argumentObject ;
      const CHAR * taskName = taskObject.getStringField( FIELD_NAME_NAME ) ;
      const rtnAlterTaskSchema & alterTask = rtnGetAlterTask( _objectType,
                                                              taskName ) ;

      PD_CHECK( alterTask.isValid(), SDB_INVALIDARG, error, PDERROR,
                "Invalid alter task name: %s", taskName ) ;
      PD_CHECK( _objectType == alterTask.getObjectType(),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid alter task, expected %s",
                alterTask.getObjectTypeName() ) ;

      SDB_ASSERT( alterTask.isValid(), "Alter task must be valid" ) ;

      argumentObject = taskObject.getObjectField( FIELD_NAME_ARGS ) ;

      rc = _createTask( alterTask, argumentObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create alter task, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERJOB__EXTRACTTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__CREATETASK, "_rtnAlterJob::_createTask" )
   INT32 _rtnAlterJob::_createTask ( const rtnAlterTaskSchema & taskSchema,
                                     const bson::BSONObj & arguments )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERJOB__CREATETASK ) ;

      rtnAlterTask * task = NULL ;

      switch ( taskSchema.getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         {
            task = SDB_OSS_NEW rtnCLCreateIDIndexTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            task = SDB_OSS_NEW rtnCLDropIDIndexTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            task = SDB_OSS_NEW rtnCLEnableShardingTask( taskSchema,
                                                        arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         {
            task = SDB_OSS_NEW rtnCLDisableShardingTask( taskSchema,
                                                         arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         {
            task = SDB_OSS_NEW rtnCLEnableCompressTask( taskSchema,
                                                        arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            task = SDB_OSS_NEW rtnCLDisableCompressTask( taskSchema,
                                                         arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            task = SDB_OSS_NEW rtnCLSetAttributeTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_DOMAIN :
         {
            task = SDB_OSS_NEW rtnCSSetDomainTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         {
            task = SDB_OSS_NEW rtnCSRemoveDomainTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CS_ENABLE_CAPPED :
         {
            task = SDB_OSS_NEW rtnCSEnableCappedTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CS_DISABLE_CAPPED :
         {
            task = SDB_OSS_NEW rtnCSDisableCappedTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            task = SDB_OSS_NEW rtnCSSetAttributeTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_ADD_GROUPS :
         {
            task = SDB_OSS_NEW rtnDomainAddGroupTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_SET_GROUPS :
         {
            task = SDB_OSS_NEW rtnDomainSetGroupTask( taskSchema, arguments ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_REMOVE_GROUPS :
         {
            task = SDB_OSS_NEW rtnDomainRemoveGroupTask( taskSchema,
                                                         arguments ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_SET_ATTRIBUTES :
         {
            task = SDB_OSS_NEW rtnDomainSetAttributeTask( taskSchema,
                                                          arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            task = SDB_OSS_NEW rtnCLCreateAutoincFieldTask( taskSchema,
                                                            arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            task = SDB_OSS_NEW rtnCLDropAutoincFieldTask( taskSchema,
                                                          arguments ) ;
            break ;
         }
         case RTN_ALTER_CL_INC_VERSION:
         {
            task = SDB_OSS_NEW rtnCLIncVersionTask( taskSchema, arguments ) ;
            break;
         }
         case RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION:
         {
            task = SDB_OSS_NEW _rtnDomainSetActiveLocationTask( taskSchema, arguments ) ;
            break;
         }
         case RTN_ALTER_DOMAIN_SET_LOCATION:
         {
            task = SDB_OSS_NEW _rtnDomainSetLocationTask( taskSchema, arguments ) ;
            break;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Unknown alter task [%d]",
                   taskSchema.getActionType() ) ;
      PD_CHECK( NULL != task, SDB_OOM, error, PDERROR,
                "Failed to allocate alter task [%s]",
                taskSchema.getActionName() ) ;

      rc = task->parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument for alter task [%s], "
                   "rc: %d", taskSchema.getActionName(), rc ) ;

      _alterTasks.push_back( task ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERJOB__CREATETASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( task ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTSETATTRTASK, "_rtnAlterJob::_extractSetAttrTask" )
   INT32 _rtnAlterJob::_extractSetAttrTask ( const bson::BSONObj & argument )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTSETATTRTASK ) ;

      const rtnAlterTaskSchema & alterTask = rtnGetSetAttrTask( _objectType ) ;

      PD_CHECK( alterTask.isValid(), SDB_INVALIDARG, error, PDERROR,
                "Invalid set attributes task" ) ;
      PD_CHECK( _objectType == alterTask.getObjectType(),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid alter task, expected %s",
                alterTask.getObjectTypeName() ) ;

      SDB_ASSERT( alterTask.isValid(), "Alter task must be valid" ) ;

      rc = _createTask( alterTask, argument ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create alter task, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERJOB__EXTSETATTRTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTRACTTASKS, "_rtnAlterJob::_extractTasks" )
   INT32 _rtnAlterJob::_extractTasks ( const BSONElement & taskObjects )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTRACTTASKS ) ;

      switch ( taskObjects.type() )
      {
         case Object :
         {
            rc = _extractTask( taskObjects.embeddedObject() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extract alter task [%s], "
                         "rc: %d", taskObjects.toString( TRUE, TRUE ).c_str(),
                         rc ) ;
            break ;
         }
         case Array :
         {
            BSONObjIterator iter( taskObjects.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement taskElement = iter.next() ;
               PD_CHECK( Object == taskElement.type(), SDB_INVALIDARG, error,
                         PDERROR, "Failed to extract alter task: "
                         "type of task should be object" ) ;

               rc = _extractTask( taskElement.embeddedObject() ) ;
               if ( SDB_OK != rc )
               {
                  if ( _options.isIgnoreException() )
                  {
                     // Ignore errors
                     PD_LOG( PDWARNING,
                             "Ignore failure to extract alter task[%s], rc: %d",
                             taskElement.toString( TRUE, TRUE).c_str(), rc ) ;
                     rc = SDB_OK ;
                  }
                  else
                  {
                     PD_LOG( PDERROR,
                             "Failed to extract alter task [%s], rc: %d",
                             taskElement.toString( TRUE, TRUE).c_str(), rc ) ;
                     if ( _alterTasks.empty() )
                     {
                        // Failed to parse first task, report the error directly
                        goto error ;
                     }
                     else
                     {
                        // Stop parsing, execute the parsed tasks
                        _parseRC = rc ;
                        rc = SDB_OK ;
                        break ;
                     }
                  }
               }
            }
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERJOB__EXTRACTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _rtnAlterJob::_clearTasks ()
   {
      for ( RTN_ALTER_TASK_LIST::iterator iterTask = _alterTasks.begin() ;
            iterTask !=_alterTasks.end() ;
            iterTask ++ )
      {
         rtnAlterTask * task = ( *iterTask ) ;
         SAFE_OSS_DELETE( task ) ;
      }
      _alterTasks.clear() ;
   }

   /*
      _rtnAlterJobHolder implement
    */
   _rtnAlterJobHolder::_rtnAlterJobHolder ()
   : _ownedJob( FALSE ),
     _alterJob( NULL )
   {
   }

   _rtnAlterJobHolder::~_rtnAlterJobHolder ()
   {
      deleteAlterJob() ;
   }

   INT32 _rtnAlterJobHolder::createAlterJob ()
   {
      INT32 rc = SDB_OK ;

      deleteAlterJob() ;

      _alterJob = SDB_OSS_NEW rtnAlterJob() ;
      PD_CHECK( NULL != _alterJob, SDB_OOM, error, PDERROR,
                "Failed to create alter job" ) ;
      _ownedJob = TRUE ;

   done :
      return rc ;

   error :
      goto done ;
   }

   void _rtnAlterJobHolder::deleteAlterJob ()
   {
      if ( NULL != _alterJob && _ownedJob )
      {
         SDB_OSS_DEL _alterJob ;
      }
      _alterJob = NULL ;
      _ownedJob = FALSE ;
   }

   void _rtnAlterJobHolder::setAlterJob ( rtnAlterJobHolder & holder,
                                          BOOLEAN getOwned )
   {
      deleteAlterJob() ;
      _alterJob = holder._alterJob ;
      if ( getOwned && holder._ownedJob )
      {
         _ownedJob = TRUE ;
         holder._ownedJob = FALSE ;
      }
   }

}
