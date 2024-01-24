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

   Source File Name = rtnAlterTask.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERTASK_HPP_
#define RTN_ALTERTASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"
#include "utilCompression.hpp"
#include "utilArguments.hpp"
#include "../bson/bson.hpp"
#include "utilGlobalID.hpp"
#include "utilUniqueID.hpp"
#include "msgDef.hpp"

namespace engine
{

   #define RTN_ALTER_TASK_FLAG_EMPTY         ( 0x00000000 )
   // The alter command should be executed from SHARD port
   #define RTN_ALTER_TASK_FLAG_SHARDONLY     ( 0x00000001 )
   // The alter command is allowed on main-collection
   #define RTN_ALTER_TASK_FLAG_MAINCLALLOW   ( 0x00000002 )
   // The alter command is using 3-phases in COORD
   #define RTN_ALTER_TASK_FLAG_3PHASE        ( 0x00000004 )
   // The alter command needs rollback when error happens
   #define RTN_ALTER_TASK_FLAG_ROLLBACK      ( 0x00000008 )
   // The alter command needs context locks in CATALOG
   #define RTN_ALTER_TASK_FLAG_CONTEXTLOCK   ( 0x00000010 )
   // The alter command needs sharding locks in CATALOG to prevent splits
   #define RTN_ALTER_TASK_FLAG_SHARDLOCK     ( 0x00000020 )
   // The alter command needs alter sequences
   #define RTN_ALTER_TASK_FLAG_SEQUENCE      ( 0x00000040 )
   // The alter command needs transaction lock
   #define RTN_ALTER_TASK_TRANS_LOCK         ( 0x00000080 )


   enum RTN_ALTER_OBJECT_TYPE
   {
      RTN_ALTER_INVALID_OBJECT = 0,
      RTN_ALTER_DATABASE,
      RTN_ALTER_COLLECTION,
      RTN_ALTER_COLLECTION_SPACE,
      RTN_ALTER_DOMAIN,
      RTN_ALTER_GROUP,
      RTN_ALTER_NODE
   } ;

   enum RTN_ALTER_ACTION_TYPE
   {
      RTN_ALTER_INVALID_ACTION = 0,
      RTN_ALTER_CL_CREATE_ID_INDEX,
      RTN_ALTER_CL_DROP_ID_INDEX,
      RTN_ALTER_CL_ENABLE_SHARDING,
      RTN_ALTER_CL_DISABLE_SHARDING,
      RTN_ALTER_CL_ENABLE_COMPRESS,
      RTN_ALTER_CL_DISABLE_COMPRESS,
      RTN_ALTER_CL_SET_ATTRIBUTES,
      RTN_ALTER_CS_SET_DOMAIN,
      RTN_ALTER_CS_REMOVE_DOMAIN,
      RTN_ALTER_CS_ENABLE_CAPPED,
      RTN_ALTER_CS_DISABLE_CAPPED,
      RTN_ALTER_CS_SET_ATTRIBUTES,
      RTN_ALTER_DOMAIN_ADD_GROUPS,
      RTN_ALTER_DOMAIN_SET_GROUPS,
      RTN_ALTER_DOMAIN_REMOVE_GROUPS,
      RTN_ALTER_DOMAIN_SET_ATTRIBUTES,
      RTN_ALTER_CL_CREATE_AUTOINC_FLD,
      RTN_ALTER_CL_DROP_AUTOINC_FLD,
      RTN_ALTER_CL_INC_VERSION,
      RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION,
      RTN_ALTER_DOMAIN_SET_LOCATION,
      RTN_ALTER_MAX_ACTION
   } ;

   class _pmdEDUCB ;

   class _rtnAlterOptions ;
   typedef class _rtnAlterOptions rtnAlterOptions ;

   class _rtnAlterInfo ;
   typedef class _rtnAlterInfo rtnAlterInfo ;

   class _rtnAlterTaskSchema ;
   typedef class _rtnAlterTaskSchema rtnAlterTaskSchema ;

   class _rtnAlterTask ;
   typedef class _rtnAlterTask rtnAlterTask ;

   /*
      _rtnAlterOptions define
    */
   class _rtnAlterOptions : public SDBObject
   {
      public :
         _rtnAlterOptions () ;
         virtual ~_rtnAlterOptions () ;

         void reset () ;
         bson::BSONObj toBSON () const ;

         OSS_INLINE void setIgnoreException ( BOOLEAN ignoreException )
         {
            _ignoreException = ignoreException ;
         }

         OSS_INLINE BOOLEAN isIgnoreException () const
         {
            return _ignoreException ;
         }

      protected :
         /// Ignore one alter's exception and continue to run next
         BOOLEAN _ignoreException ;
   } ;

   /*
      _rtnAlterInfo define
    */
   class _rtnAlterInfo : public SDBObject
   {
      struct cmp_str
      {
         bool operator() (const char *a, const char *b) const
         {
            return std::strcmp(a,b)<0 ;
         }
      } ;
      typedef ossPoolMap<const CHAR*, utilIdxUniqueID, cmp_str> MAP_IDXNAME_ID ;

      public :
         _rtnAlterInfo () {}
         virtual ~_rtnAlterInfo () {}

         INT32 init( const BSONObj& obj ) ;
         BSONObj toBSON() const ;

         INT32 getIndexInfoByCL( const CHAR* collection,
                                 BSONObj& indexInfo ) const ;

         utilIdxUniqueID getIdxUniqueID( const CHAR* collection,
                                         const CHAR* indexName ) const ;

      protected :
         INT32 _addIdxUniqueID( const CHAR* collection,
                                const CHAR* indexName,
                                utilIdxUniqueID indexUniqID ) ;

      protected :
         BSONObj _obj ;
         // < collection name, <index name, index unique id> >
         ossPoolMap<const CHAR*, MAP_IDXNAME_ID, cmp_str> _clMap ;
   } ;

   /*
      _rtnAlterTaskArgument define
    */
   class _rtnAlterTaskArgument ;
   typedef class _rtnAlterTaskArgument rtnAlterTaskArgument ;

   class _rtnAlterTaskArgument
   {
      public :
         _rtnAlterTaskArgument () ;
         _rtnAlterTaskArgument ( const bson::BSONObj & argument ) ;
         _rtnAlterTaskArgument ( const rtnAlterTaskArgument & argument ) ;
         virtual ~_rtnAlterTaskArgument () ;

         rtnAlterTaskArgument & operator = ( const rtnAlterTaskArgument & argument ) ;

         virtual INT32 parseArgument () = 0 ;

         OSS_INLINE bson::BSONObj getArgument () const
         {
            return _argument ;
         }

         OSS_INLINE UINT32 getArgumentMask () const
         {
            return _argumentMask ;
         }

         OSS_INLINE BOOLEAN testArgumentMask ( UINT32 fields ) const
         {
            return OSS_BIT_TEST( _argumentMask, fields ) ? TRUE : FALSE ;
         }

         OSS_INLINE void setArgumentMask ( UINT32 fields )
         {
            OSS_BIT_SET( _argumentMask, fields ) ;
         }

         OSS_INLINE void parsedArgumentMask ( UINT32 fields, UINT32 count = 1 )
         {
            setArgumentMask( fields ) ;
            _argumentCount += count ;
         }

         OSS_INLINE UINT32 getArgumentCount () const
         {
            return _argumentCount ;
         }

      protected :
         bson::BSONObj  _argument ;
         UINT32         _argumentMask ;
         UINT32         _argumentCount ;
   } ;

   /*
      _rtnAlterTaskSchema define
    */
   class _rtnAlterTaskSchema : public SDBObject
   {
      public :
         _rtnAlterTaskSchema () ;
         _rtnAlterTaskSchema ( const rtnAlterTaskSchema & schema ) ;
         _rtnAlterTaskSchema ( const CHAR * actionName,
                               RTN_ALTER_OBJECT_TYPE objectType,
                               RTN_ALTER_ACTION_TYPE actionType,
                               UINT32 flagValue ) ;
         virtual ~_rtnAlterTaskSchema () ;

         rtnAlterTaskSchema & operator = ( const rtnAlterTaskSchema & task ) ;

         BOOLEAN isValid () const ;

         const CHAR * getObjectTypeName () const ;
         const CHAR * getCommandName () const ;

         OSS_INLINE const CHAR * getActionName () const
         {
            return _actionName ;
         }

         OSS_INLINE RTN_ALTER_OBJECT_TYPE getObjectType () const
         {
            return _objectType ;
         }

         OSS_INLINE RTN_ALTER_ACTION_TYPE getActionType () const
         {
            return _actionType ;
         }

         OSS_INLINE void resetFlagValue ( UINT32 flagValue )
         {
            _flagValue = flagValue ;
         }

         OSS_INLINE UINT32 getFlagValue () const
         {
            return _flagValue ;
         }

         OSS_INLINE void clearFlagValue ()
         {
            _flagValue = RTN_ALTER_TASK_FLAG_EMPTY ;
         }

         OSS_INLINE void setFlags ( UINT32 flags )
         {
            OSS_BIT_SET( _flagValue, flags ) ;
         }

         OSS_INLINE BOOLEAN testFlags ( UINT32 flags ) const
         {
            return ( OSS_BIT_TEST( _flagValue, flags ) ? TRUE : FALSE ) ;
         }

         OSS_INLINE void clearFlags ( UINT32 flags )
         {
            OSS_BIT_CLEAR( _flagValue, flags ) ;
         }

      protected :
         const CHAR *            _actionName ;
         RTN_ALTER_OBJECT_TYPE   _objectType ;
         RTN_ALTER_ACTION_TYPE   _actionType ;
         UINT32                  _flagValue ;
   } ;

   /*
      _rtnAlterTask define
    */
   class _rtnAlterTask : public _rtnAlterTaskSchema,
                         public _rtnAlterTaskArgument
   {
      public :
         _rtnAlterTask ( const rtnAlterTaskSchema & schema,
                         const bson::BSONObj & argument ) ;
         virtual ~_rtnAlterTask () ;

         OSS_INLINE const rtnAlterTaskSchema * getSchema () const
         {
            return _taskSchema ;
         }

         virtual INT32 parseArgument ()
         {
            return SDB_OK ;
         }

         INT32 toCMDMessage ( CHAR ** ppBuffer,
                              INT32 * bufferSize,
                              const CHAR * objectName,
                              const bson::BSONObj & options,
                              const bson::BSONObj * pAlterInfo,
                              UINT64 reqID,
                              _pmdEDUCB * cb ) const ;

         bson::BSONObj toBSON ( const CHAR * objectName,
                                const bson::BSONObj & options,
                                const bson::BSONObj * pAlterInfo ) const ;

      protected :
         const rtnAlterTaskSchema * _taskSchema ;
   } ;

   /*
      _rtnCLShardingArgument define
    */
   class _rtnCLShardingArgument ;
   typedef class _rtnCLShardingArgument rtnCLShardingArgument ;

   class _rtnCLShardingArgument : public _rtnAlterTaskArgument
   {
      public :
         _rtnCLShardingArgument () ;
         _rtnCLShardingArgument ( const bson::BSONObj & argument,
                                  BOOLEAN checkShardingKey ) ;
         _rtnCLShardingArgument ( const rtnCLShardingArgument & argument ) ;
         virtual ~_rtnCLShardingArgument () ;

         rtnCLShardingArgument & operator = ( const rtnCLShardingArgument & argument ) ;

         virtual INT32 parseArgument () ;

         OSS_INLINE BOOLEAN isSharding () const
         {
            return !_shardingKey.isEmpty() ;
         }

         OSS_INLINE void setHashSharding( BOOLEAN hashSharding )
         {
            _hashSharding = hashSharding ;
         }

         OSS_INLINE BOOLEAN isHashSharding () const
         {
            return _hashSharding ;
         }

         OSS_INLINE void setShardingKey ( const bson::BSONObj & shardingKey )
         {
            _shardingKey = shardingKey ;
         }

         OSS_INLINE bson::BSONObj getShardingKey () const
         {
            return _shardingKey ;
         }

         OSS_INLINE void setPartition ( UINT32 partition )
         {
            _partition = partition ;
         }

         OSS_INLINE UINT32 getPartition () const
         {
            return _partition ;
         }

         OSS_INLINE void setEnsureShardingIndex ( BOOLEAN ensureShardingIndex )
         {
            _ensureShardingIndex = ensureShardingIndex ;
         }

         OSS_INLINE BOOLEAN isEnsureShardingIndex () const
         {
            return _ensureShardingIndex ;
         }

         OSS_INLINE BOOLEAN isAutoSplit () const
         {
            return _autoSplit ;
         }

      protected :
         BOOLEAN        _checkShardingKey ;
         BOOLEAN        _hashSharding ;
         bson::BSONObj  _shardingKey ;
         UINT32         _partition ;
         BOOLEAN        _ensureShardingIndex ;
         BOOLEAN        _autoSplit ;
   } ;

   /*
      _rtnCLCompressArgument define
    */
   class _rtnCLCompressArgument ;
   typedef class _rtnCLCompressArgument rtnCLCompressArgument ;

   class _rtnCLCompressArgument : public _rtnAlterTaskArgument
   {
      public :
         _rtnCLCompressArgument () ;
         _rtnCLCompressArgument ( const bson::BSONObj & argument,
                                  BOOLEAN checkCompressed ) ;
         _rtnCLCompressArgument ( const rtnCLCompressArgument & argument ) ;
         virtual ~_rtnCLCompressArgument () ;

         rtnCLCompressArgument & operator = ( const rtnCLCompressArgument & argument ) ;

         virtual INT32 parseArgument () ;

         void setCompress ( UTIL_COMPRESSOR_TYPE compressorType ) ;

         OSS_INLINE BOOLEAN isCompressed () const
         {
            return _compressed ;
         }

         OSS_INLINE const CHAR * getCompressionName () const
         {
            return _compressorName ;
         }

         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressorType () const
         {
            return _compressorType ;
         }

      protected :
         BOOLEAN              _checkCompressed ;
         BOOLEAN              _compressed ;
         const CHAR *         _compressorName ;
         UTIL_COMPRESSOR_TYPE _compressorType ;
   } ;

   /*
      _rtnCLExtOptionArgument define
    */
   class _rtnCLExtOptionArgument ;
   typedef class _rtnCLExtOptionArgument rtnCLExtOptionArgument ;

   class _rtnCLExtOptionArgument : public _rtnAlterTaskArgument
   {
      public :
         _rtnCLExtOptionArgument () ;
         _rtnCLExtOptionArgument ( const bson::BSONObj & argument ) ;
         _rtnCLExtOptionArgument ( const rtnCLExtOptionArgument & argument ) ;
         virtual ~_rtnCLExtOptionArgument () ;

         rtnCLExtOptionArgument & operator = ( const rtnCLExtOptionArgument & argument ) ;

         INT32 parseArgument () ;

         INT32 toBSON ( const bson::BSONObj & curExtOptions,
                        bson::BSONObj & extOptions ) const ;

         OSS_INLINE UINT64 getMaxSize () const
         {
            return _maxSize ;
         }

         OSS_INLINE UINT64 getMaxRec () const
         {
            return _maxRec ;
         }

         OSS_INLINE BOOLEAN isOverWrite () const
         {
            return _overWrite ;
         }

      protected :
         UINT64   _maxSize ;
         UINT64   _maxRec ;
         BOOLEAN  _overWrite ;
   } ;

   /*
      _rtnCLRepairCheckArgument define
    */
   class _rtnCLRepairCheckArgument ;
   typedef class _rtnCLRepairCheckArgument rtnCLRepairCheckArgument ;

   class _rtnCLRepairCheckArgument : public _rtnAlterTaskArgument
   {
      public :
         _rtnCLRepairCheckArgument() ;
         _rtnCLRepairCheckArgument( const bson::BSONObj &argument ) ;
         _rtnCLRepairCheckArgument(
                                const _rtnCLRepairCheckArgument &argument ) ;
         virtual ~_rtnCLRepairCheckArgument() ;

         _rtnCLRepairCheckArgument& operator =(
                                 const _rtnCLRepairCheckArgument &argument ) ;

         INT32 parseArgument() ;

         BOOLEAN isRepairCheck() const ;

      protected :
         BOOLEAN _repairCheck ;
   } ;

   /*
      _rtnCLAutoincFieldArgument define
    */
   class _rtnCLAutoincFieldArgument;
   typedef class _rtnCLAutoincFieldArgument rtnCLAutoincFieldArgument;
   typedef std::vector<rtnCLAutoincFieldArgument*> autoIncFieldsList ;
   class _rtnCLAutoincFieldArgument : public SDBObject,
                                      public _rtnAlterTaskArgument
   {
      public :
         _rtnCLAutoincFieldArgument () ;
         _rtnCLAutoincFieldArgument ( const bson::BSONObj & argument ) ;
         _rtnCLAutoincFieldArgument (
                                 const rtnCLAutoincFieldArgument & argument ) ;
         virtual ~_rtnCLAutoincFieldArgument () ;

         rtnCLAutoincFieldArgument & operator = (
                                 const rtnCLAutoincFieldArgument & argument ) ;

         virtual INT32 parseArgument () ;

         OSS_INLINE void setFieldName ( const CHAR * name )
         {
            _fieldName = name ;
         }

         OSS_INLINE const CHAR * getFieldName () const
         {
            return _fieldName;
         }

         OSS_INLINE const CHAR * getSequenceName () const
         {
            return _seqName ;
         }

         OSS_INLINE const CHAR * getGenerated () const
         {
            return _generated ;
         }

         OSS_INLINE utilSequenceID getID () const
         {
            return _ID ;
         }

         OSS_INLINE void setGenerated ( const CHAR * generated )
         {
            _generated = generated ;
         }

         OSS_INLINE void setID ( const utilSequenceID ID )
         {
            _ID = ID ;
         }

      protected:
         const CHAR *         _fieldName ;      // field name
         const CHAR *         _seqName ;        // seq name
         utilSequenceID       _ID ;             // ID
         const CHAR *         _generated ;      // generated
   };

   /*
      _rtnAlterCLTask define
    */
   class _rtnAlterCLTask : public _rtnAlterTask
   {
      public :
         _rtnAlterCLTask ( const rtnAlterTaskSchema & schema,
                           const bson::BSONObj & argument ) ;
         virtual ~_rtnAlterCLTask () ;
   } ;

   /*
      _rtnCLCreateIDIndexTask define
    */
   class _rtnCLCreateIDIndexTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLCreateIDIndexTask ( const rtnAlterTaskSchema & schema,
                                   const bson::BSONObj & argument ) ;
         virtual ~_rtnCLCreateIDIndexTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE INT32 getSortBufferSize () const
         {
            return _sortBufferSize ;
         }

      protected :
         INT32 _sortBufferSize ;
   } ;

   typedef class _rtnCLCreateIDIndexTask rtnCLCreateIDIndexTask ;

   /*
      _rtnCLDropIDIndexTask define
    */
   class _rtnCLDropIDIndexTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLDropIDIndexTask ( const rtnAlterTaskSchema & schema,
                                 const bson::BSONObj & argument ) ;
         virtual ~_rtnCLDropIDIndexTask () ;
   } ;

   typedef class _rtnCLDropIDIndexTask rtnCLDropIDIndexTask ;

   /*
      _rtnCLCreateAutoincrementTask define
    */
   class _rtnCLCreateAutoincFieldTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLCreateAutoincFieldTask ( const rtnAlterTaskSchema & schema,
                                   const bson::BSONObj & argument ) ;
         virtual ~_rtnCLCreateAutoincFieldTask () ;
         virtual INT32 parseArgument () ;
         OSS_INLINE BOOLEAN containAutoincArgument () const
         {
            return !_autoIncFieldList.empty() ;
         }

         OSS_INLINE const autoIncFieldsList & getAutoincrementArgument () const
         {
            return _autoIncFieldList ;
         }

      protected:
          autoIncFieldsList _autoIncFieldList ;
   } ;

   typedef class _rtnCLCreateAutoincFieldTask rtnCLCreateAutoincFieldTask ;

   /*
      _rtnCLDropAutoincTask define
    */
   class _rtnCLDropAutoincFieldTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLDropAutoincFieldTask ( const rtnAlterTaskSchema & schema,
                                 const bson::BSONObj & argument ) ;
         virtual ~_rtnCLDropAutoincFieldTask () ;
         virtual INT32 parseArgument () ;
         OSS_INLINE BOOLEAN containAutoincArgument () const
         {
            return !_autoIncFieldList.empty() ;
         }

         OSS_INLINE const autoIncFieldsList & getAutoincrementArgument () const
         {
            return _autoIncFieldList ;
         }

      protected:
          autoIncFieldsList _autoIncFieldList ;
   } ;

   typedef class _rtnCLDropAutoincFieldTask rtnCLDropAutoincFieldTask ;

   /*
      _rtnCLIncVersionTask define
    */
   class _rtnCLIncVersionTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLIncVersionTask ( const rtnAlterTaskSchema & schema,
                                 const bson::BSONObj & argument ) ;
         virtual ~_rtnCLIncVersionTask () ;
   } ;

   typedef class _rtnCLIncVersionTask rtnCLIncVersionTask ;

   /*
      _rtnCLEnableShardingTask define
    */
   class _rtnCLEnableShardingTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLEnableShardingTask ( const rtnAlterTaskSchema & schema,
                                    const bson::BSONObj & argument ) ;
         virtual ~_rtnCLEnableShardingTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const rtnCLShardingArgument & getShardingArgument () const
         {
            return _shardingArgument ;
         }

      protected :
         rtnCLShardingArgument _shardingArgument ;
   } ;

   typedef class _rtnCLEnableShardingTask rtnCLEnableShardingTask ;

   /*
      _rtnCLDisableShardingTask define
    */
   class _rtnCLDisableShardingTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLDisableShardingTask ( const rtnAlterTaskSchema & schema,
                                     const bson::BSONObj & argument ) ;
         virtual ~_rtnCLDisableShardingTask () ;
   } ;

   typedef class _rtnCLDisableShardingTask rtnCLDisableShardingTask ;

   /*
      _rtnCLEnableCompressTask define
    */
   class _rtnCLEnableCompressTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLEnableCompressTask ( const rtnAlterTaskSchema & schema,
                                    const bson::BSONObj & argument ) ;
         virtual ~_rtnCLEnableCompressTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const rtnCLCompressArgument & getCompressArgument () const
         {
            return _compressArgument ;
         }

      protected :
         rtnCLCompressArgument _compressArgument ;
   } ;

   typedef class _rtnCLEnableCompressTask rtnCLEnableCompressTask ;

   /*
      _rtnCLDisableCompressTask define
    */
   class _rtnCLDisableCompressTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLDisableCompressTask ( const rtnAlterTaskSchema & schema,
                                     const bson::BSONObj & argument ) ;
         virtual ~_rtnCLDisableCompressTask () ;
   } ;

   typedef class _rtnCLDisableCompressTask rtnCLDisableCompressTask ;

   /*
      _rtnCLSetAttributeTask define
    */
   class _rtnCLSetAttributeTask : public _rtnAlterCLTask
   {
      public :
         _rtnCLSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnCLSetAttributeTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE BOOLEAN containShardingArgument () const
         {
            return _shardingArgument.getArgumentMask() != RTN_ALTER_TASK_FLAG_EMPTY ;
         }

         OSS_INLINE const rtnCLShardingArgument & getShardingArgument () const
         {
            return _shardingArgument ;
         }

         OSS_INLINE const rtnCLRepairCheckArgument &getRepairCheckArgument() const
         {
            return _repairCheckArgument ;
         }

         OSS_INLINE BOOLEAN containCompressArgument () const
         {
            return _compressArgument.getArgumentMask() !=
                                                RTN_ALTER_TASK_FLAG_EMPTY ;
         }

         OSS_INLINE BOOLEAN containRepairCheckArgument () const
         {
            return _repairCheckArgument.getArgumentMask() !=
                                                RTN_ALTER_TASK_FLAG_EMPTY ;
         }

         OSS_INLINE const rtnCLCompressArgument & getCompressArgument () const
         {
            return _compressArgument ;
         }

         OSS_INLINE BOOLEAN containExtOptionArgument () const
         {
            return _extOptionArgument.getArgumentMask() !=
                                                RTN_ALTER_TASK_FLAG_EMPTY ;
         }

         OSS_INLINE const rtnCLExtOptionArgument & getExtOptionArgument () const
         {
            return _extOptionArgument ;
         }

         OSS_INLINE BOOLEAN containAutoincArgument () const
         {
            return !_autoIncFieldList.empty() ;
         }

         OSS_INLINE const autoIncFieldsList & getAutoincFieldArgument () const
         {
            return _autoIncFieldList ;
         }

         OSS_INLINE BOOLEAN isAutoRebalance () const
         {
            return _autoRebalance ;
         }

         OSS_INLINE BOOLEAN isAutoIndexID () const
         {
            return _autoIndexID ;
         }

         OSS_INLINE INT32 getReplSize () const
         {
            return _replSize ;
         }

         OSS_INLINE SDB_CONSISTENCY_STRATEGY getConsistencyStrategy () const
         {
            return _consistencyStrategy ;
         }

         OSS_INLINE BOOLEAN isStrictDataMode () const
         {
            return _strictDataMode ;
         }

         OSS_INLINE BOOLEAN isNoTrans() const
         {
            return _noTrans ;
         }

         OSS_INLINE utilIdxUniqueID getIdIdxUniqueID () const
         {
            return _idIdxUniqID ;
         }

      private:
         BOOLEAN _conflictCheck() ;

      protected :
         rtnCLShardingArgument      _shardingArgument ;
         rtnCLCompressArgument      _compressArgument ;
         rtnCLRepairCheckArgument   _repairCheckArgument ;
         rtnCLExtOptionArgument     _extOptionArgument ;
         autoIncFieldsList          _autoIncFieldList ;

         BOOLEAN         _autoRebalance ;
         BOOLEAN         _autoIndexID ;
         utilIdxUniqueID _idIdxUniqID ;
         INT32           _replSize ;
         SDB_CONSISTENCY_STRATEGY _consistencyStrategy ;
         BOOLEAN         _strictDataMode ;
         BOOLEAN         _noTrans ;
   } ;

   typedef class _rtnCLSetAttributeTask rtnCLSetAttributeTask ;

   /*
      _rtnAlterCSTask define
    */
   class _rtnAlterCSTask : public _rtnAlterTask
   {
      public :
         _rtnAlterCSTask ( const rtnAlterTaskSchema & schema,
                           const bson::BSONObj & argument ) ;
         virtual ~_rtnAlterCSTask () ;
   } ;

   typedef class _rtnAlterCSTask rtnAlterCSTask ;

   /*
      _rtnCSSetDomainTask define
    */
   class _rtnCSSetDomainTask : public _rtnAlterCSTask
   {
      public :
         _rtnCSSetDomainTask ( const rtnAlterTaskSchema & schema,
                               const bson::BSONObj & argument ) ;
         virtual ~_rtnCSSetDomainTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const CHAR * getDomain () const
         {
            return _domain ;
         }

      protected :
         INT32 _parseArgument () ;

      protected :
         const CHAR * _domain ;
   } ;

   typedef class _rtnCSSetDomainTask rtnCSSetDomainTask ;

   /*
      _rtnCSRemoveDomainTask define
    */
   class _rtnCSRemoveDomainTask : public _rtnAlterCSTask
   {
      public :
         _rtnCSRemoveDomainTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnCSRemoveDomainTask () ;
   } ;

   typedef class _rtnCSRemoveDomainTask rtnCSRemoveDomainTask ;

   /*
      _rtnCSEnableCappedTask define
    */
   class _rtnCSEnableCappedTask : public _rtnAlterCSTask
   {
      public :
         _rtnCSEnableCappedTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnCSEnableCappedTask () ;
   } ;

   typedef class _rtnCSEnableCappedTask rtnCSEnableCappedTask ;

   /*
      _rtnCSDisableCappedTask define
    */
   class _rtnCSDisableCappedTask : public _rtnAlterCSTask
   {
      public :
         _rtnCSDisableCappedTask ( const rtnAlterTaskSchema & schema,
                                   const bson::BSONObj & argument ) ;
         virtual ~_rtnCSDisableCappedTask () ;
   } ;

   typedef class _rtnCSDisableCappedTask rtnCSDisableCappedTask ;

   /*
      _rtnCSSetAttributeTask define
    */
   class _rtnCSSetAttributeTask : public _rtnCSSetDomainTask
   {
      public :
         _rtnCSSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnCSSetAttributeTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE INT32 getPageSize () const
         {
            return _pageSize ;
         }

         OSS_INLINE INT32 getLobPageSize () const
         {
            return _lobPageSize ;
         }

         OSS_INLINE BOOLEAN isCapped () const
         {
            return _capped ;
         }

      protected :
         INT32 _pageSize ;
         INT32 _lobPageSize ;
         BOOLEAN _capped ;
   } ;

   typedef class _rtnCSSetAttributeTask rtnCSSetAttributeTask ;

   /*
      _rtnAlterDomainTask define
    */

   typedef ossPoolList< const CHAR * > RTN_DOMAIN_GROUP_LIST ;

   class _rtnAlterDomainTask : public _rtnAlterTask
   {
      public :
         _rtnAlterDomainTask ( const rtnAlterTaskSchema & schema,
                               const bson::BSONObj & argument ) ;
         virtual ~_rtnAlterDomainTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const RTN_DOMAIN_GROUP_LIST & getGroups () const
         {
            return _groups ;
         }

      protected :
         RTN_DOMAIN_GROUP_LIST _groups ;
   } ;

   typedef class _rtnAlterDomainTask rtnAlterDomainTask ;

   /*
      _rtnDomainAddGroupTask define
    */
   class _rtnDomainAddGroupTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainAddGroupTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainAddGroupTask () ;

         virtual INT32 parseArgument () ;
   } ;

   typedef class _rtnDomainAddGroupTask rtnDomainAddGroupTask ;

   /*
      _rtnDomainRemoveGroupTask define
    */
   class _rtnDomainRemoveGroupTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainRemoveGroupTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainRemoveGroupTask () ;

         virtual INT32 parseArgument () ;
   } ;

   typedef class _rtnDomainRemoveGroupTask rtnDomainRemoveGroupTask ;

   /*
      _rtnDomainSetGroupTask define
    */
   class _rtnDomainSetGroupTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainSetGroupTask ( const rtnAlterTaskSchema & schema,
                                  const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainSetGroupTask () ;

         virtual INT32 parseArgument () ;
   } ;

   typedef class _rtnDomainSetGroupTask rtnDomainSetGroupTask ;

   /*
      _rtnDomainSetActiveLocationTask define
    */
   class _rtnDomainSetActiveLocationTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainSetActiveLocationTask ( const rtnAlterTaskSchema & schema,
                                       const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainSetActiveLocationTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const CHAR * getActiveLocation() const
         {
            return _pLocation ;
         }

      protected:
         const CHAR* _pLocation ;
   } ;

   typedef class _rtnDomainSetActiveLocationTask rtnDomainSetActiveLocationTask ;

   /*
      _rtnDomainSetLocationTask define
    */
   class _rtnDomainSetLocationTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainSetLocationTask ( const rtnAlterTaskSchema & schema,
                                    const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainSetLocationTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE const CHAR * getHostName() const
         {
            return _pHostName ;
         }

         OSS_INLINE const CHAR * getLocation() const
         {
            return _pLocation ;
         }

      protected:
         const CHAR* _pHostName ;
         const CHAR* _pLocation ;
   } ;

   typedef class _rtnDomainSetLocationTask rtnDomainSetLocationTask ;

   /*
      _rtnDomainSetAttributeTask define
    */
   class _rtnDomainSetAttributeTask : public _rtnAlterDomainTask
   {
      public :
         _rtnDomainSetAttributeTask () ;
         _rtnDomainSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                      const bson::BSONObj & argument ) ;
         virtual ~_rtnDomainSetAttributeTask () ;

         virtual INT32 parseArgument () ;

         OSS_INLINE void setGroups ( const RTN_DOMAIN_GROUP_LIST & groups )
         {
            _groups = groups ;
         }

         OSS_INLINE void setAutoSplit ( BOOLEAN autoSplit )
         {
            _autoSplit = autoSplit ;
         }

         OSS_INLINE BOOLEAN isAutoSplit () const
         {
            return _autoSplit ;
         }

         OSS_INLINE void setAutoRebalance ( BOOLEAN autoRebalance )
         {
            _autoRebalance = autoRebalance ;
         }

         OSS_INLINE BOOLEAN isAutoRebalance () const
         {
            return _autoRebalance ;
         }

      protected :
         BOOLEAN  _autoSplit ;
         BOOLEAN  _autoRebalance ;
   } ;

   typedef class _rtnDomainSetAttributeTask rtnDomainSetAttributeTask ;

}

#endif // RTN_ALTERTASK_HPP_
