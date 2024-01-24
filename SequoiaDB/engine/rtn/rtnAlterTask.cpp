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

   Source File Name = rtnAlterTask.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnAlterTask.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include "msgDef.hpp"
#include "msgMessage.hpp"
#include "pmdEDU.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _rtnAlterOptions implement
    */
   _rtnAlterOptions::_rtnAlterOptions ()
   : _ignoreException( FALSE )
   {
   }

   _rtnAlterOptions::~_rtnAlterOptions ()
   {
   }

   void _rtnAlterOptions::reset ()
   {
      _ignoreException = FALSE ;
   }

   BSONObj _rtnAlterOptions::toBSON () const
   {
      BSONObjBuilder builder ;
      builder.appendBool( FIELD_NAME_IGNORE_EXCEPTION, _ignoreException ) ;
      return builder.obj() ;
   }

   /*
      _rtnAlterInfo implement
    */
   INT32 _rtnAlterInfo::init( const BSONObj& obj )
   {
      INT32 rc = SDB_OK ;

      /*{ Index: [ { Collection: "foo.bar", IndexDef: xxx },
      *            { Collection: "foo.bar", IndexDef: xxx },
      *            { Collection: "foo.ba1", IndexDef: xxx }
      *          ]
      * }
      */
      try
      {
         _obj = obj.getOwned() ;

         BSONElement ele = _obj.getField( FIELD_NAME_INDEX ) ;
         if ( ele.eoo() )
         {
            goto done ;
         }

         PD_CHECK( Array == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid field[%s] type[%d]",
                   FIELD_NAME_INDEX, ele.type() ) ;

         BSONObjIterator it( ele.embeddedObject() ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            const CHAR* collection = NULL ;
            const CHAR* indexName = NULL ;
            utilIdxUniqueID indexUniqID = UTIL_UNIQUEID_NULL ;
            BSONObj indexDef ;

            PD_CHECK( Object == e.type() , SDB_INVALIDARG, error, PDERROR,
                      "Invalid type[%d]", e.type() ) ;

            BSONObj idxObj = e.Obj() ;

            e = idxObj.getField( FIELD_NAME_COLLECTION ) ;
            PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid field[%s] type[%d]",
                      FIELD_NAME_COLLECTION, e.type() ) ;
            collection = e.valuestrsafe() ;

            e = idxObj.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
            PD_CHECK( Object == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid field[%s] type[%d]",
                      IXM_FIELD_NAME_INDEX_DEF, e.type() ) ;
            indexDef = e.Obj() ;

            e = indexDef.getField( IXM_FIELD_NAME_NAME ) ;
            PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid field[%s] type[%d]",
                      IXM_FIELD_NAME_NAME, e.type() ) ;
            indexName = e.valuestrsafe() ;

            e = indexDef.getField( IXM_FIELD_NAME_UNIQUEID ) ;
            PD_CHECK( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid field[%s] type[%d]",
                      IXM_FIELD_NAME_UNIQUEID, e.type() ) ;
            indexUniqID = (utilIdxUniqueID)e.numberLong() ;

            rc = _addIdxUniqueID( collection, indexName, indexUniqID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to add index uniqueid, rc: %d",
                         rc ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _rtnAlterInfo::toBSON() const
   {
      return _obj ;
   }

   utilIdxUniqueID _rtnAlterInfo::getIdxUniqueID( const CHAR* collection,
                                                  const CHAR* indexName ) const
   {
      ossPoolMap<const CHAR*, MAP_IDXNAME_ID, cmp_str>::const_iterator it =
                                                _clMap.find( collection ) ;
      if ( it != _clMap.end() )
      {
         const MAP_IDXNAME_ID& idxMap = it->second ;
         MAP_IDXNAME_ID::const_iterator i = idxMap.find( indexName ) ;
         if ( i != idxMap.end() )
         {
            return i->second ;
         }
      }
      return UTIL_UNIQUEID_NULL ;
   }

   INT32 _rtnAlterInfo::getIndexInfoByCL( const CHAR* collection,
                                          BSONObj& indexInfo ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         ossPoolMap<const CHAR*, MAP_IDXNAME_ID, cmp_str>::const_iterator it =
                                                   _clMap.find( collection ) ;
         if ( it != _clMap.end() )
         {
           /* { Index: [ { Collection: "foo.bar", IndexDef: xxx },
            *            { Collection: "foo.bar", IndexDef: xxx }, ...
            *          ]
            * }
            */
            BSONObjBuilder builder ;
            BSONArrayBuilder sub( builder.subarrayStart( FIELD_NAME_INDEX ) ) ;

            const MAP_IDXNAME_ID& idxMap = it->second ;
            for ( MAP_IDXNAME_ID::const_iterator i = idxMap.begin() ;
                  i != idxMap.end() ; i++ )
            {
               sub.append( BSON( FIELD_NAME_COLLECTION << collection <<
                                 IXM_FIELD_NAME_INDEX_DEF <<
                                 BSON( IXM_FIELD_NAME_NAME << i->first <<
                                       IXM_FIELD_NAME_UNIQUEID <<
                                       (INT64)(i->second) ) ) ) ;
            }

            sub.done() ;
            indexInfo = builder.obj() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterInfo::_addIdxUniqueID( const CHAR* collection,
                                         const CHAR* indexName,
                                         utilIdxUniqueID indexUniqID )
   {
      try
      {
         ossPoolMap<const CHAR*, MAP_IDXNAME_ID, cmp_str>::iterator it =
                                             _clMap.find( collection ) ;
         if ( it == _clMap.end() )
         {
            MAP_IDXNAME_ID idxMap ;
            idxMap[ indexName ] = indexUniqID ;
            _clMap[ collection ] = idxMap ;
         }
         else
         {
            (it->second)[ indexName ] = indexUniqID ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return ossException2RC( &e ) ;
      }

      return SDB_OK ;
   }

   /*
      _rtnAlterTaskArgument implement
    */
   _rtnAlterTaskArgument::_rtnAlterTaskArgument ()
   : _argumentMask( UTIL_ARG_FIELD_EMPTY ),
     _argumentCount( 0 )
   {
   }

   _rtnAlterTaskArgument::_rtnAlterTaskArgument ( const BSONObj & argument )
   : _argument( argument ),
     _argumentMask( UTIL_ARG_FIELD_EMPTY ),
     _argumentCount( 0 )
   {
   }

   _rtnAlterTaskArgument::_rtnAlterTaskArgument ( const rtnAlterTaskArgument & argument )
   : _argument( argument._argument ),
     _argumentMask( argument._argumentMask ),
     _argumentCount( argument._argumentCount )
   {
   }

   _rtnAlterTaskArgument::~_rtnAlterTaskArgument ()
   {
   }

   rtnAlterTaskArgument & _rtnAlterTaskArgument::operator = ( const rtnAlterTaskArgument & argument )
   {
      _argument = argument._argument ;
      _argumentMask = argument._argumentMask ;
      _argumentCount = argument._argumentCount ;
      return ( *this ) ;
   }

   /*
      _rtnAlterTaskSchema implement
    */
   _rtnAlterTaskSchema::_rtnAlterTaskSchema ()
   : _actionName( NULL ),
     _objectType( RTN_ALTER_INVALID_OBJECT ),
     _actionType( RTN_ALTER_INVALID_ACTION ),
     _flagValue( RTN_ALTER_TASK_FLAG_EMPTY )
   {
   }

   _rtnAlterTaskSchema::_rtnAlterTaskSchema ( const rtnAlterTaskSchema & schema )
   : _actionName( schema._actionName ),
     _objectType( schema._objectType ),
     _actionType( schema._actionType ),
     _flagValue( schema._flagValue )
   {
   }

   _rtnAlterTaskSchema::_rtnAlterTaskSchema ( const CHAR * actionName,
                                              RTN_ALTER_OBJECT_TYPE objectType,
                                              RTN_ALTER_ACTION_TYPE actionType,
                                              UINT32 flagValue )
   : _actionName( actionName ),
     _objectType( objectType ),
     _actionType( actionType ),
     _flagValue( flagValue )
   {
   }

   _rtnAlterTaskSchema::~_rtnAlterTaskSchema ()
   {
   }

   rtnAlterTaskSchema & _rtnAlterTaskSchema::operator = ( const rtnAlterTaskSchema & task )
   {
      _actionName = task._actionName ;
      _objectType = task._objectType ;
      _actionType = task._actionType ;
      resetFlagValue( task.getFlagValue() ) ;
      return ( *this ) ;
   }

   BOOLEAN _rtnAlterTaskSchema::isValid () const
   {
      return ( NULL != _actionName &&
               RTN_ALTER_INVALID_OBJECT != _objectType &&
               RTN_ALTER_INVALID_ACTION != _actionType ) ;
   }

   const CHAR * _rtnAlterTaskSchema::getObjectTypeName () const
   {
      const CHAR * objectName = NULL ;

      switch ( _objectType )
      {
         case RTN_ALTER_DATABASE :
            objectName = SDB_CATALOG_DB ;
            break ;
         case RTN_ALTER_COLLECTION :
            objectName = SDB_CATALOG_CL ;
            break ;
         case RTN_ALTER_COLLECTION_SPACE :
            objectName = SDB_CATALOG_CS ;
            break ;
         case RTN_ALTER_DOMAIN :
            objectName = SDB_CATALOG_DOMAIN ;
            break ;
         case RTN_ALTER_GROUP :
            objectName = SDB_CATALOG_GROUP ;
            break ;
         case RTN_ALTER_NODE :
            objectName = SDB_CATALOG_NODE ;
            break ;
         default :
            SDB_ASSERT( FALSE, "Invalid object" ) ;
            objectName = SDB_CATALOG_UNKNOWN ;
            break ;
      }

      return objectName ;
   }

   const CHAR * _rtnAlterTaskSchema::getCommandName () const
   {
      const CHAR * commandName = NULL ;

      switch ( _objectType )
      {
         case RTN_ALTER_DATABASE :
            commandName = CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION ;
            break ;
         case RTN_ALTER_COLLECTION :
            commandName = CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION ;
            break ;
         case RTN_ALTER_COLLECTION_SPACE :
            commandName = CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION_SPACE ;
            break ;
         case RTN_ALTER_DOMAIN :
            commandName = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN ;
            break ;
         default :
            SDB_ASSERT( FALSE, "Invalid command" ) ;
            commandName = CMD_ADMIN_PREFIX "unknown" ;
            break ;
      }

      return commandName ;
   }

   /*
      _rtnAlterTask implement
    */
   _rtnAlterTask::_rtnAlterTask ( const rtnAlterTaskSchema & schema,
                                  const BSONObj & argument )
   : _rtnAlterTaskSchema( schema ),
     _rtnAlterTaskArgument( argument )
   {
   }

   _rtnAlterTask::~_rtnAlterTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERTASKRUNNER_TOCMDMSG, "_rtnAlterTask::toCMDMessage" )
   INT32 _rtnAlterTask::toCMDMessage ( CHAR ** ppBuffer,
                                       INT32 * bufferSize,
                                       const CHAR * objectName,
                                       const BSONObj & options,
                                       const BSONObj * pAlterInfo,
                                       UINT64 reqID,
                                       _pmdEDUCB * cb ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERTASKRUNNER_TOCMDMSG ) ;

      BSONObj alterObject = toBSON( objectName, options, pAlterInfo ) ;

      rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize, getCommandName(),
                                alterObject, BSONObj(), BSONObj(), BSONObj(),
                                reqID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build alter command, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERTASKRUNNER_TOCMDMSG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   BSONObj _rtnAlterTask::toBSON ( const CHAR * objectName,
                                   const bson::BSONObj & options,
                                   const bson::BSONObj * pAlterInfo ) const
   {
      BSONObjBuilder builder ;
      BSONObj alterObject ;

      builder.append( FIELD_NAME_ALTER_TYPE, getObjectTypeName() ) ;
      builder.append( FIELD_NAME_VERSION, SDB_ALTER_VERSION ) ;
      builder.append( FIELD_NAME_NAME, objectName ) ;
      builder.append( FIELD_NAME_OPTIONS, options ) ;

      BSONObjBuilder taskBuilder( builder.subobjStart( FIELD_NAME_ALTER ) ) ;
      taskBuilder.append( FIELD_NAME_NAME, getActionName() ) ;
      taskBuilder.append( FIELD_NAME_ARGS, _argument ) ;
      taskBuilder.done() ;

      if ( pAlterInfo )
      {
         builder.append( FIELD_NAME_ALTER_INFO, *pAlterInfo ) ;
      }

      return builder.obj() ;
   }

   /*
      _rtnCLShardingArgument implement
    */
   _rtnCLShardingArgument::_rtnCLShardingArgument ()
   : _rtnAlterTaskArgument(),
     _checkShardingKey( FALSE ),
     _hashSharding( TRUE ),
     _partition( SDB_SHARDING_PARTITION_DEFAULT ),
     _ensureShardingIndex( TRUE ),
     _autoSplit( FALSE )
   {
   }

   _rtnCLShardingArgument::_rtnCLShardingArgument ( const BSONObj & argument,
                                                    BOOLEAN checkShardingKey )
   : _rtnAlterTaskArgument( argument ),
     _checkShardingKey( checkShardingKey ),
     _hashSharding( TRUE ),
     _partition( SDB_SHARDING_PARTITION_DEFAULT ),
     _ensureShardingIndex( TRUE ),
     _autoSplit( FALSE )
   {
   }

   _rtnCLShardingArgument::_rtnCLShardingArgument ( const rtnCLShardingArgument & argument )
   : _rtnAlterTaskArgument( argument ),
     _checkShardingKey( argument._checkShardingKey ),
     _hashSharding( argument._hashSharding ),
     _shardingKey( argument._shardingKey ),
     _partition( argument._partition ),
     _ensureShardingIndex( argument._ensureShardingIndex ),
     _autoSplit( argument._autoSplit )
   {
   }

   _rtnCLShardingArgument::~_rtnCLShardingArgument ()
   {
   }

   rtnCLShardingArgument & _rtnCLShardingArgument::operator = ( const rtnCLShardingArgument & argument )
   {
      _rtnAlterTaskArgument::operator =( argument ) ;

      _checkShardingKey = argument._checkShardingKey ;
      _hashSharding = argument._hashSharding ;
      _shardingKey = argument._shardingKey ;
      _partition = argument._partition ;
      _ensureShardingIndex = argument._ensureShardingIndex ;
      _autoSplit = argument._autoSplit ;

      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLSHARDINGARG_PARSEARG, "_rtnCLShardingArgument::parseArgument" )
   INT32 _rtnCLShardingArgument::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLSHARDINGARG_PARSEARG ) ;

      BSONElement argElement ;

      if ( _checkShardingKey )
      {
         // Report no sharding key
         PD_CHECK( _argument.hasField( FIELD_NAME_SHARDINGKEY ),
                   SDB_NO_SHARDINGKEY, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_SHARDINGKEY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_SHARDINGKEY ) )
      {
         argElement = _argument.getField( FIELD_NAME_SHARDINGKEY ) ;
         PD_CHECK( Object == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_SHARDINGKEY ) ;
         _shardingKey = argElement.embeddedObject() ;
         PD_CHECK( _ixmIndexKeyGen::validateKeyDef( _shardingKey ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Sharding key [%s] definition is invalid",
                   _shardingKey.toString().c_str() ) ;
         parsedArgumentMask( UTIL_CL_SHDKEY_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_SHARDTYPE ) )
      {
         const CHAR * shardingType = NULL ;
         argElement = _argument.getField( FIELD_NAME_SHARDTYPE ) ;
         PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_SHARDTYPE ) ;
         shardingType = argElement.valuestr() ;

         if ( 0 == ossStrcmp( shardingType, FIELD_NAME_SHARDTYPE_HASH ) )
         {
            _hashSharding = TRUE ;
         }
         else if ( 0 == ossStrcmp( shardingType, FIELD_NAME_SHARDTYPE_RANGE ) )
         {
            _hashSharding = FALSE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field [%s]: %s or %s is expected, "
                    "but value is %s", FIELD_NAME_SHARDTYPE,
                    FIELD_NAME_SHARDTYPE_HASH, FIELD_NAME_SHARDTYPE_RANGE,
                    shardingType ) ;
            goto error ;
         }

         parsedArgumentMask( UTIL_CL_SHDTYPE_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_PARTITION ) )
      {
         argElement = _argument.getField( FIELD_NAME_PARTITION ) ;
         PD_CHECK( NumberInt == argElement.type(), SDB_INVALIDARG, error,
                   PDERROR, "Failed to get field [%s]", FIELD_NAME_PARTITION ) ;
         _partition = argElement.numberInt() ;

         // must be the power of 2
         PD_CHECK( ossIsPowerOf2( (UINT32)_partition ), SDB_INVALIDARG, error,
                   PDERROR, "Field [%s] value must be power of 2",
                   FIELD_NAME_PARTITION ) ;
         PD_CHECK( _partition >= SDB_SHARDING_PARTITION_MIN &&
                   _partition <= SDB_SHARDING_PARTITION_MAX,
                   SDB_INVALIDARG, error, PDERROR, "Field [%s] value should "
                   "between in [%d, %d]", FIELD_NAME_PARTITION,
                   SDB_SHARDING_PARTITION_MIN, SDB_SHARDING_PARTITION_MAX ) ;
         PD_CHECK( _hashSharding, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse field [%s]", FIELD_NAME_PARTITION ) ;

         parsedArgumentMask( UTIL_CL_PARTITION_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_ENSURE_SHDINDEX ) )
      {
         argElement = _argument.getField( FIELD_NAME_ENSURE_SHDINDEX ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error,
                   PDERROR, "Failed to get field [%s]",
                   FIELD_NAME_ENSURE_SHDINDEX ) ;
         _ensureShardingIndex = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_ENSURESHDIDX_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_DOMAIN_AUTO_SPLIT ) )
      {
         argElement = _argument.getField( FIELD_NAME_DOMAIN_AUTO_SPLIT ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_DOMAIN_AUTO_SPLIT ) ;
         _autoSplit = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_AUTOSPLIT_FIELD ) ;
      }

      if ( _autoSplit )
      {
         PD_CHECK( _hashSharding, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse sharding argument: "
                   "AutoSplit should be used in hash sharding only" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLSHARDINGARG_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCLCompressArgument implement
    */
   _rtnCLCompressArgument::_rtnCLCompressArgument ()
   : _checkCompressed( FALSE ),
     _compressed( FALSE ),
     _compressorName( utilCompressType2String( UTIL_COMPRESSOR_INVALID ) ),
     _compressorType( UTIL_COMPRESSOR_INVALID )
   {
   }

   _rtnCLCompressArgument::_rtnCLCompressArgument ( const BSONObj & argument,
                                                    BOOLEAN checkCompressed )
   : _rtnAlterTaskArgument( argument ),
     _checkCompressed( checkCompressed ),
     _compressed( FALSE ),
     _compressorName( utilCompressType2String( UTIL_COMPRESSOR_SNAPPY ) ),
     _compressorType( UTIL_COMPRESSOR_INVALID )
   {
   }

   _rtnCLCompressArgument::_rtnCLCompressArgument ( const rtnCLCompressArgument & argument )
   : _rtnAlterTaskArgument( argument ),
     _checkCompressed( argument._checkCompressed ),
     _compressed( argument._compressed ),
     _compressorName( argument._compressorName ),
     _compressorType( argument._compressorType )
   {
   }

   _rtnCLCompressArgument::~_rtnCLCompressArgument ()
   {
   }

   rtnCLCompressArgument & _rtnCLCompressArgument::operator = ( const rtnCLCompressArgument & argument )
   {
      _rtnAlterTaskArgument::operator =( argument ) ;

      _checkCompressed = argument._checkCompressed ;
      _compressed = argument._compressed ;
      _compressorName = argument._compressorName ;
      _compressorType = argument._compressorType ;

      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLCOMPRESSARG_PARSEARG, "_rtnCLCompressArgument::parseArgument" )
   INT32 _rtnCLCompressArgument::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLCOMPRESSARG_PARSEARG ) ;

      BSONElement argElement ;

      if ( _checkCompressed && _argument.hasField( FIELD_NAME_COMPRESSED ) )
      {
         argElement = _argument.getField( FIELD_NAME_COMPRESSED ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_COMPRESSED ) ;
         _compressed = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_COMPRESSED_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_COMPRESSIONTYPE ) )
      {
         argElement = _argument.getField( FIELD_NAME_COMPRESSIONTYPE ) ;
         PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_COMPRESSIONTYPE ) ;
         _compressorName = argElement.valuestr() ;

         if ( 0 == ossStrcmp( VALUE_NAME_SNAPPY, _compressorName ) )
         {
            _compressorType = UTIL_COMPRESSOR_SNAPPY ;
         }
         else if ( 0 == ossStrcmp( VALUE_NAME_LZW, _compressorName ) )
         {
            _compressorType = UTIL_COMPRESSOR_LZW ;
         }
         else if ( 0 == ossStrcmp( VALUE_NAME_LZ4, _compressorName ) )
         {
            _compressorType = UTIL_COMPRESSOR_LZ4 ;
         }
         else if ( 0 == ossStrcmp( VALUE_NAME_ZLIB, _compressorName ) )
         {
            _compressorType = UTIL_COMPRESSOR_ZLIB ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Failed to get field [%s]", FIELD_NAME_COMPRESSIONTYPE ) ;
         }
         parsedArgumentMask( UTIL_CL_COMPRESSTYPE_FIELD ) ;
      }

      if ( getArgumentMask() == ( UTIL_CL_COMPRESSED_FIELD |
                                  UTIL_CL_COMPRESSTYPE_FIELD ) )
      {
         // Both Compressed and CompresssionType are set
         if ( !_compressed && _compressorType != UTIL_COMPRESSOR_INVALID )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "[%s] and [%s] are conflicts",
                    FIELD_NAME_COMPRESSED, FIELD_NAME_COMPRESSIONTYPE ) ;
            goto error ;
         }
      }
      else if ( getArgumentMask() == UTIL_CL_COMPRESSED_FIELD )
      {
         // Only Compressed is set
         if ( _compressed )
         {
            setCompress( UTIL_COMPRESSOR_SNAPPY ) ;
         }
         else
         {
            setCompress( UTIL_COMPRESSOR_INVALID ) ;
         }
      }
      else if ( getArgumentMask() == UTIL_CL_COMPRESSTYPE_FIELD )
      {
         // Only CompresssionType is set
         if ( _compressorType == UTIL_COMPRESSOR_INVALID )
         {
            _compressed = FALSE ;
         }
         else
         {
            _compressed = TRUE ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLCOMPRESSARG_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _rtnCLCompressArgument::setCompress ( UTIL_COMPRESSOR_TYPE compressorType )
   {
      _compressed = ( compressorType == UTIL_COMPRESSOR_INVALID ) ? FALSE :
                                                                    TRUE ;
      _compressorType = compressorType ;
      _compressorName = utilCompressType2String( compressorType ) ;
   }

   /*
      _rtnCLRepairCheckArgument implement
    */
   _rtnCLRepairCheckArgument::_rtnCLRepairCheckArgument()
   : _rtnAlterTaskArgument(),
     _repairCheck( FALSE )
   {
   }

   _rtnCLRepairCheckArgument::_rtnCLRepairCheckArgument(
                                           const bson::BSONObj &argument )
   : _rtnAlterTaskArgument( argument ),
     _repairCheck( FALSE )
   {
   }

   _rtnCLRepairCheckArgument::_rtnCLRepairCheckArgument(
                                     const _rtnCLRepairCheckArgument &argument )
   : _rtnAlterTaskArgument( argument ),
     _repairCheck( argument._repairCheck )
   {
   }

   _rtnCLRepairCheckArgument::~_rtnCLRepairCheckArgument()
   {
   }

   _rtnCLRepairCheckArgument& _rtnCLRepairCheckArgument::operator = (
                                    const _rtnCLRepairCheckArgument &argument )
   {
      _rtnAlterTaskArgument::operator =( argument ) ;
      _repairCheck = argument._repairCheck ;

      return ( *this ) ;
   }

   INT32 _rtnCLRepairCheckArgument::parseArgument()
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = _argument.getField( FIELD_NAME_REPARECHECK ) ;
      if ( !ele.eoo() )
      {
         PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_REPARECHECK ) ;

         _repairCheck = ele.booleanSafe() ;
         parsedArgumentMask( UTIL_CL_REPAIRCHECK_FIELD ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnCLRepairCheckArgument::isRepairCheck() const
   {
      return _repairCheck ;
   }

   /*
      _rtnCLExtOptionArgument implement
    */
   _rtnCLExtOptionArgument::_rtnCLExtOptionArgument ()
   : _maxSize( 0 ),
     _maxRec( 0 ),
     _overWrite( FALSE )
   {
   }

   _rtnCLExtOptionArgument::_rtnCLExtOptionArgument ( const BSONObj & argument )
   : _rtnAlterTaskArgument( argument ),
     _maxSize( 0 ),
     _maxRec( 0 ),
     _overWrite( FALSE )
   {
   }

   _rtnCLExtOptionArgument::_rtnCLExtOptionArgument ( const rtnCLExtOptionArgument & argument )
   : _rtnAlterTaskArgument( argument ),
     _maxSize( argument._maxSize ),
     _maxRec( argument._maxRec ),
     _overWrite( argument._overWrite )
   {
   }

   _rtnCLExtOptionArgument::~_rtnCLExtOptionArgument ()
   {
   }

   rtnCLExtOptionArgument & _rtnCLExtOptionArgument::operator = ( const rtnCLExtOptionArgument & argument )
   {
      _rtnAlterTaskArgument::operator =( argument ) ;

      _maxSize = argument._maxSize ;
      _maxRec = argument._maxRec ;
      _overWrite = argument._overWrite ;

      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLCAPARG_PARSEARG, "_rtnCLExtOptionArgument::parseArgument" )
   INT32 _rtnCLExtOptionArgument::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLCAPARG_PARSEARG ) ;

      BSONElement argElement ;

      if ( _argument.hasField( FIELD_NAME_SIZE ) )
      {
         argElement = _argument.getField( FIELD_NAME_SIZE ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_SIZE ) ;
         // Round up to 32MB
         _maxSize = (UINT64)ossRoundUpToMultipleX( argElement.numberLong() << 20,
                                                   DMS_MAX_CL_SIZE_ALIGN_SIZE ) ;
         parsedArgumentMask( UTIL_CL_MAXSIZE_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_MAX ) )
      {
         argElement = _argument.getField( FIELD_NAME_MAX ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_MAX ) ;
         PD_CHECK( argElement.numberLong() >= 0,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]: should be positive value",
                   FIELD_NAME_MAX ) ;
         _maxRec = (UINT64)argElement.numberLong() ;
         parsedArgumentMask( UTIL_CL_MAXREC_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_OVERWRITE ) )
      {
         argElement = _argument.getField( FIELD_NAME_OVERWRITE ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_OVERWRITE ) ;
         _overWrite = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_OVERWRITE_FIELD ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLCAPARG_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLCAPARG_TOBSON, "_rtnCLExtOptionArgument::toBSON" )
   INT32 _rtnCLExtOptionArgument::toBSON ( const BSONObj & curExtOptions,
                                        BSONObj & extOptions ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLCAPARG_TOBSON ) ;

      BSONObjBuilder builder ;

      // Size field
      if ( testArgumentMask( UTIL_CL_MAXSIZE_FIELD ) )
      {
         builder.append( FIELD_NAME_SIZE, (INT64)_maxSize ) ;
      }
      else
      {
         PD_CHECK( curExtOptions.hasField( FIELD_NAME_SIZE ),
                   SDB_SYS, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_SIZE ) ;
         builder.append( curExtOptions.getField( FIELD_NAME_SIZE ) ) ;
      }

      // Max field
      if ( testArgumentMask( UTIL_CL_MAXREC_FIELD ) )
      {
         builder.append( FIELD_NAME_MAX, (INT64)_maxRec ) ;
      }
      else
      {
         PD_CHECK( curExtOptions.hasField( FIELD_NAME_MAX ),
                   SDB_SYS, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_MAX ) ;
         builder.append( curExtOptions.getField( FIELD_NAME_MAX ) ) ;
      }

      // OverWrite field
      if ( testArgumentMask( UTIL_CL_OVERWRITE_FIELD ) )
      {
         builder.appendBool( FIELD_NAME_OVERWRITE, _overWrite ) ;
      }
      else
      {
         PD_CHECK( curExtOptions.hasField( FIELD_NAME_OVERWRITE ),
                   SDB_SYS, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_OVERWRITE ) ;
         builder.append( curExtOptions.getField( FIELD_NAME_OVERWRITE ) ) ;
      }

      extOptions = builder.obj() ;

   done :
      PD_TRACE_EXITRC( SDB__RTNCLCAPARG_TOBSON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnAlterCLTask implement
    */
   _rtnAlterCLTask::_rtnAlterCLTask ( const rtnAlterTaskSchema & schema,
                                      const BSONObj & argument )
   : _rtnAlterTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_COLLECTION == schema.getObjectType(),
                  "schema is invalid" ) ;
   }

   _rtnAlterCLTask::~_rtnAlterCLTask ()
   {
   }

   /*
      _rtnCLCreateIDIndexTask implement
    */
   _rtnCLCreateIDIndexTask::_rtnCLCreateIDIndexTask ( const rtnAlterTaskSchema & schema,
                                                      const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument ),
     _sortBufferSize( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE )
   {
      SDB_ASSERT( RTN_ALTER_CL_CREATE_ID_INDEX == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLCreateIDIndexTask::~_rtnCLCreateIDIndexTask ()
   {
   }

   INT32 _rtnCLCreateIDIndexTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      BSONElement argElement ;

      if ( _argument.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         argElement = _argument.getField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         PD_CHECK( NumberInt == argElement.type() && argElement.Int() >= 0,
                   SDB_INVALIDARG, error, PDERROR, "Failed to get field [%s]",
                   IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         _sortBufferSize = argElement.Int() ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCLDropIDIndexTask implement
    */
   _rtnCLDropIDIndexTask::_rtnCLDropIDIndexTask ( const rtnAlterTaskSchema & schema,
                                                  const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CL_DROP_ID_INDEX == schema.getActionType(),
                  "schema is invalid" ) ;
      setFlags( RTN_ALTER_TASK_TRANS_LOCK ) ;
   }

   _rtnCLDropIDIndexTask::~_rtnCLDropIDIndexTask ()
   {
   }

   /*
      _rtnCLAutoincrementFieldArgument implement
    */
   _rtnCLAutoincFieldArgument::_rtnCLAutoincFieldArgument ()
   : _rtnAlterTaskArgument(),
     _fieldName( NULL ),
     _seqName( NULL ),
     _ID( UTIL_SEQUENCEID_NULL ),
     _generated( NULL )
   {
   }

   _rtnCLAutoincFieldArgument::_rtnCLAutoincFieldArgument (
                                                   const BSONObj & argument )
   : _rtnAlterTaskArgument( argument ),
     _fieldName( NULL ),
     _seqName( NULL ),
     _ID( UTIL_SEQUENCEID_NULL ),
     _generated( NULL )
   {
   }

   _rtnCLAutoincFieldArgument::_rtnCLAutoincFieldArgument (
                                  const rtnCLAutoincFieldArgument & argument )
   : _rtnAlterTaskArgument( argument ),
     _fieldName( argument._fieldName ),
     _seqName( argument._seqName ),
     _ID( argument._ID ),
     _generated( argument._generated )
   {
   }

   _rtnCLAutoincFieldArgument::~_rtnCLAutoincFieldArgument()
   {
   }

   rtnCLAutoincFieldArgument & _rtnCLAutoincFieldArgument::operator =
                              ( const rtnCLAutoincFieldArgument & argument )
   {
      _rtnAlterTaskArgument::operator =( argument ) ;
      _fieldName = argument._fieldName ;
      _seqName = argument._seqName ;
      _ID = argument._ID ;
      _generated = argument._generated ;
      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLAUTOINCFIELDARG_PARSEARG, "_rtnCLAutoincFieldArgument::parseArgument" )
   INT32 _rtnCLAutoincFieldArgument::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLAUTOINCFIELDARG_PARSEARG ) ;

      BSONElement argElement ;

      PD_CHECK( _argument.hasField( FIELD_NAME_AUTOINC_FIELD ), SDB_INVALIDARG,
                error, PDERROR, "Failed to get field [%s]",
                FIELD_NAME_AUTOINC_FIELD ) ;

      argElement = _argument.getField( FIELD_NAME_AUTOINC_FIELD ) ;
      PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
               "Failed to get field [%s]", FIELD_NAME_AUTOINC_FIELD ) ;

      setFieldName( argElement.valuestrsafe() );

      if( _argument.hasField( FIELD_NAME_INCREMENT ) )
      {
         argElement = _argument.getField( FIELD_NAME_INCREMENT ) ;
         PD_CHECK( NumberInt== argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_INCREMENT ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_INCREMENT_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_START_VALUE ) )
      {
         argElement = _argument.getField( FIELD_NAME_START_VALUE ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_START_VALUE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_MIN_VALUE ) )
      {
         argElement = _argument.getField( FIELD_NAME_MIN_VALUE ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_MIN_VALUE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_MINVALUE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_MAX_VALUE ) )
      {
         argElement = _argument.getField( FIELD_NAME_MAX_VALUE ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_MAX_VALUE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_MAXVALUE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_CURRENT_VALUE ) )
      {
         argElement = _argument.getField( FIELD_NAME_CURRENT_VALUE ) ;
         PD_CHECK( argElement.isNumber(), SDB_INVALIDARG, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_CURRENT_VALUE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_CURVALUE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_CACHE_SIZE ) )
      {
         argElement = _argument.getField( FIELD_NAME_CACHE_SIZE ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_CACHE_SIZE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_CACHESIZE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_ACQUIRE_SIZE ) )
      {
         argElement = _argument.getField( FIELD_NAME_ACQUIRE_SIZE ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_ACQUIRE_SIZE ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_CYCLED ) )
      {
         argElement = _argument.getField( FIELD_NAME_CYCLED ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_CYCLED ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_CYCLED_FIELD ) ;
      }

      if( _argument.hasField( FIELD_NAME_GENERATED ) )
      {
         argElement = _argument.getField( FIELD_NAME_GENERATED ) ;
         PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_GENERATED ) ;
         setGenerated( argElement.valuestrsafe() ) ;
         parsedArgumentMask( UTIL_CL_AUTOINC_GENERATED_FIELD ) ;
      }

      parsedArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ) ;

      PD_CHECK( getArgumentCount() == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNCLAUTOINCFIELDARG_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCLCreateAutoincFieldTask implement
    */
   _rtnCLCreateAutoincFieldTask::_rtnCLCreateAutoincFieldTask ( const rtnAlterTaskSchema & schema,
                                                                const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
   }

   _rtnCLCreateAutoincFieldTask::~_rtnCLCreateAutoincFieldTask ()
   {
      for( UINT32 i = 0 ; i < _autoIncFieldList.size() ; i++)
      {
         SAFE_OSS_DELETE( _autoIncFieldList[ i ] ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLCREATEAUTOINCFIELDTASK_PARSEARG, "_rtnCLCreateAutoincFieldTask::parseArgument" )
   INT32 _rtnCLCreateAutoincFieldTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLCREATEAUTOINCFIELDTASK_PARSEARG ) ;

      BSONElement ele ;
      BSONObj autoIncObject ;
      rtnCLAutoincFieldArgument *autoIncField = NULL ;

      try
      {
         PD_CHECK( _argument.hasField( FIELD_NAME_AUTOINCREMENT ),
                   SDB_INVALIDARG, error, PDERROR, "Failed to get field[%s]",
                   FIELD_NAME_AUTOINCREMENT ) ;
         ele = _argument.getField( FIELD_NAME_AUTOINCREMENT ) ;
         if( ele.type() == Array )
         {
            BSONObjIterator it( ele.embeddedObject() );
            while( it.more() )
            {
               BSONElement e ;
               e = it.next() ;
               PD_CHECK( Object == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Invalid argument[%s], rc:%d",
                         _argument.toString(false,false).c_str(), rc  ) ;
               autoIncObject = e.Obj() ;
               PD_CHECK( autoIncObject.hasField( FIELD_NAME_AUTOINC_FIELD ),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to get field[%s]", FIELD_NAME_AUTOINC_FIELD ) ;

               autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( autoIncObject ) ;
               PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                         "Failed to allocate autoincrement field argument" ) ;

               rc = autoIncField->parseArgument() ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to parse autoincrement argument, rc: %d",
                            rc ) ;
               PD_CHECK( !autoIncField->testArgumentMask( UTIL_CL_AUTOINC_CURVALUE_FIELD ),
                         SDB_INVALIDARG, error, PDERROR,
                         "Option[%s] not support when create autoincrement "
                         "field", FIELD_NAME_CURRENT_VALUE ) ;
               _autoIncFieldList.push_back( autoIncField ) ;
               autoIncField = NULL;
            }
         }
         else if( ele.type() == Object )
         {
            autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( ele.Obj() ) ;
            PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                      "Failed to allocate autoincrement field argument" ) ;

            rc = autoIncField->parseArgument() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse autoincrement argument, "
                         "rc: %d", rc ) ;
            _autoIncFieldList.push_back( autoIncField ) ;
            autoIncField = NULL;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid argument[%s], rc:%d",
                    _argument.toString(false,false).c_str(), rc ) ;
         }

         parsedArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse arguments for create "
                 "autoincfieldtask[%s], exception=%s",
                 _argument.toString( false, false ).c_str(), e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLCREATEAUTOINCFIELDTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( autoIncField );
      goto done ;
   }

   /*
      _rtnCLDropAutoincFieldTask implement
    */
   _rtnCLDropAutoincFieldTask::_rtnCLDropAutoincFieldTask ( const rtnAlterTaskSchema & schema,
                                                            const bson::BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
   }

   _rtnCLDropAutoincFieldTask::~_rtnCLDropAutoincFieldTask ()
   {
      for( UINT32 i = 0 ; i < _autoIncFieldList.size() ; i++)
      {
         SAFE_OSS_DELETE( _autoIncFieldList[ i ] ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPAUTOINCFIELDTASK_PARSEARG, "_rtnCLDropAutoincFieldTask::parseArgument" )
   INT32 _rtnCLDropAutoincFieldTask::parseArgument()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLCREATEAUTOINCFIELDTASK_PARSEARG ) ;

      BSONElement ele ;
      BSONObj autoIncObject ;
      rtnCLAutoincFieldArgument *autoIncField = NULL ;

      try
      {
         PD_CHECK( _argument.hasField( FIELD_NAME_AUTOINC_FIELD ), SDB_INVALIDARG,
                   error, PDERROR, "Failed to get field [%s]",
                   FIELD_NAME_AUTOINC_FIELD ) ;
         ele = _argument.getField( FIELD_NAME_AUTOINC_FIELD ) ;
         PD_CHECK( ele.type() == bson::String ||
                   ele.type() == bson::Array, SDB_INVALIDARG,
                   error, PDERROR, "Failed to get field [%s]",
                   FIELD_NAME_AUTOINC_FIELD ) ;
         if ( ele.type() == bson::String )
         {
            autoIncObject = _argument ;
            autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( autoIncObject ) ;
            PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                      "Failed to allocate autoincrement field argument" ) ;

            rc = autoIncField->parseArgument() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse autoincrement "
                         " argument, rc: %d", rc ) ;
            PD_CHECK( autoIncField->getArgumentCount() == 1,
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse argument: contain unknown fields[%s]",
                      _argument.toString( false, false ).c_str() ) ;
            _autoIncFieldList.push_back( autoIncField ) ;
            autoIncField = NULL;
         }
         else if( ele.type() == bson::Array )
         {
            BSONObjIterator it( ele.embeddedObject() );
            while( it.more() )
            {
               BSONElement field = it.next() ;
               BSONObjBuilder argbuilder ;

               PD_CHECK( field.type() == bson::String, SDB_INVALIDARG,
                        error, PDERROR, "Failed to get field [%s]",
                         FIELD_NAME_AUTOINCREMENT ) ;
               argbuilder.append( FIELD_NAME_AUTOINC_FIELD , field.String() ) ;
               autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( argbuilder.obj() ) ;
               PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                         "Failed to allocate autoincrement field argument" ) ;
               rc = autoIncField->parseArgument() ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to parse autoincrement argument, rc: %d",
                            rc ) ;
               PD_CHECK( autoIncField->getArgumentCount() == 1,
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse argument: contain unknown fields[%s]",
                         _argument.toString( false, false ).c_str() ) ;
               _autoIncFieldList.push_back( autoIncField ) ;
               autoIncField = NULL;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid argument[%s], rc:%d",
                    _argument.toString(false,false).c_str(), rc ) ;
         }

         parsedArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse arguments for "
                 "drop autoincfieldtask[%s], exception=%s",
                 _argument.toString( false, false ).c_str(), e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLCREATEAUTOINCFIELDTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( autoIncField );
      goto done ;

   }

   /*
      _rtnCLIncVersionTask implement
    */
   _rtnCLIncVersionTask::_rtnCLIncVersionTask ( const rtnAlterTaskSchema & schema,
                                                const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CL_INC_VERSION == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLIncVersionTask::~_rtnCLIncVersionTask ()
   {
   }

   /*
      _rtnCLEnableShardingTask implement
    */
   _rtnCLEnableShardingTask::_rtnCLEnableShardingTask ( const rtnAlterTaskSchema & schema,
                                                        const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument ),
     _shardingArgument( argument, TRUE )
   {
      SDB_ASSERT( RTN_ALTER_CL_ENABLE_SHARDING == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLEnableShardingTask::~_rtnCLEnableShardingTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLENABLESHARDINGTASK_PARSEARG, "_rtnCLEnableShardingTask::parseArgument" )
   INT32 _rtnCLEnableShardingTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLENABLESHARDINGTASK_PARSEARG ) ;

      rc = _shardingArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse sharding argument, rc: %d", rc ) ;
      parsedArgumentMask( _shardingArgument.getArgumentMask(),
                          _shardingArgument.getArgumentCount() ) ;

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

      if ( getArgumentMask() == UTIL_CL_SHDKEY_FIELD )
      {
         setFlags( RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCLENABLESHARDINGTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCLDisableShardingTask implement
    */
   _rtnCLDisableShardingTask::_rtnCLDisableShardingTask ( const rtnAlterTaskSchema & schema,
                                                          const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CL_DISABLE_SHARDING == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLDisableShardingTask::~_rtnCLDisableShardingTask ()
   {
   }

   /*
      _rtnCLEnableCompressTask implement
    */
   _rtnCLEnableCompressTask::_rtnCLEnableCompressTask ( const rtnAlterTaskSchema & schema,
                                                        const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument ),
     _compressArgument( argument, FALSE )
   {
      SDB_ASSERT( RTN_ALTER_CL_ENABLE_COMPRESS == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLEnableCompressTask::~_rtnCLEnableCompressTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCLENABLECOMPRESSTASK_PARSEARG, "_rtnCLEnableCompressTask::parseArgument" )
   INT32 _rtnCLEnableCompressTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCLENABLECOMPRESSTASK_PARSEARG ) ;

      rc = _compressArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse compression argument, rc: %d", rc ) ;

      if ( !_compressArgument.isCompressed() )
      {
         _compressArgument.setCompress( UTIL_COMPRESSOR_SNAPPY ) ;
         _compressArgument.setArgumentMask( UTIL_CL_COMPRESSED_FIELD |
                                            UTIL_CL_COMPRESSTYPE_FIELD ) ;
      }
      parsedArgumentMask( _compressArgument.getArgumentMask(),
                          _compressArgument.getArgumentCount() ) ;

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNCLENABLECOMPRESSTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCLDisableCompressTask implement
    */
   _rtnCLDisableCompressTask::_rtnCLDisableCompressTask ( const rtnAlterTaskSchema & schema,
                                                          const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CL_DISABLE_COMPRESS == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLDisableCompressTask::~_rtnCLDisableCompressTask ()
   {
   }

   /*
      _rtnCLSetAttributeTask implement
    */
   _rtnCLSetAttributeTask::_rtnCLSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnAlterCLTask( schema, argument ),
     _shardingArgument( argument, FALSE ),
     _compressArgument( argument, TRUE ),
     _repairCheckArgument( argument ),
     _extOptionArgument( argument ),
     _autoRebalance( FALSE ),
     _autoIndexID( TRUE ),
     _idIdxUniqID( UTIL_UNIQUEID_NULL ),
     _replSize( 1 ),
     _consistencyStrategy( SDB_CONSISTENCY_PRY_LOC_MAJOR ),
     _strictDataMode( TRUE ),
     _noTrans( FALSE )
   {
      SDB_ASSERT( RTN_ALTER_CL_SET_ATTRIBUTES == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCLSetAttributeTask::~_rtnCLSetAttributeTask ()
   {
      if( containAutoincArgument() )
      {
         for( UINT32 i = 0 ; i < _autoIncFieldList.size() ; i++)
         {
            SAFE_OSS_DELETE( _autoIncFieldList[ i ] ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLSETATTRTASK_PARSEARG, "_rtnCLSetAttributeTask::parseArgument" )
   INT32 _rtnCLSetAttributeTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLSETATTRTASK_PARSEARG ) ;

      BSONElement argElement ;
      rtnCLAutoincFieldArgument *autoIncField = NULL;

      rc = _shardingArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse sharding argument, rc: %d", rc ) ;

      if ( UTIL_ARG_FIELD_EMPTY != _shardingArgument.getArgumentMask() )
      {
         if ( UTIL_CL_AUTOSPLIT_FIELD != _shardingArgument.getArgumentMask() )
         {
            setFlags( RTN_ALTER_TASK_FLAG_ROLLBACK |
                      RTN_ALTER_TASK_FLAG_SHARDLOCK |
                      RTN_ALTER_TASK_FLAG_3PHASE |
                      RTN_ALTER_TASK_FLAG_CONTEXTLOCK ) ;
         }
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      parsedArgumentMask( _shardingArgument.getArgumentMask(),
                          _shardingArgument.getArgumentCount() ) ;

      rc = _compressArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse compression argument, rc: %d", rc ) ;

      if ( UTIL_ARG_FIELD_EMPTY != _compressArgument.getArgumentMask() )
      {
         setFlags( RTN_ALTER_TASK_FLAG_3PHASE ) ;
      }

      parsedArgumentMask( _compressArgument.getArgumentMask(),
                          _compressArgument.getArgumentCount() ) ;

      rc = _repairCheckArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse access mode argument, rc: %d", rc ) ;

      parsedArgumentMask( _repairCheckArgument.getArgumentMask(),
                          _repairCheckArgument.getArgumentCount() ) ;

      rc = _extOptionArgument.parseArgument() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse capped argument, rc: %d", rc ) ;

      if ( UTIL_ARG_FIELD_EMPTY != _extOptionArgument.getArgumentMask() )
      {
         setFlags( RTN_ALTER_TASK_FLAG_3PHASE ) ;
      }

      parsedArgumentMask( _extOptionArgument.getArgumentMask(),
                          _extOptionArgument.getArgumentCount() ) ;

      if ( _argument.hasField( FIELD_NAME_AUTOINCREMENT ) )
      {
         argElement = _argument.getField( FIELD_NAME_AUTOINCREMENT ) ;
         PD_CHECK( bson::Object == argElement.type() ||
                   bson::Array == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR, "Failed to get field [%s]",
                   FIELD_NAME_AUTOINCREMENT ) ;
         if( bson::Object == argElement.type() )
         {
             PD_CHECK( argElement.Obj().hasField( FIELD_NAME_AUTOINC_FIELD ),
                       SDB_INVALIDARG, error, PDERROR,
                       "Failed to get field [%s]", FIELD_NAME_AUTOINCREMENT ) ;
            autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( argElement.Obj() ) ;
            PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                      "Failed to allocate autoincrement field argument" ) ;

            rc = autoIncField->parseArgument() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse autoincrement argument, "
                         "rc: %d", rc ) ;
            _autoIncFieldList.push_back( autoIncField ) ;
            autoIncField = NULL;
         }
         else if(bson::Array == argElement.type() )
         {
            BSONObjIterator it( argElement.embeddedObject() ) ;
            while( it.more() )
            {
               BSONElement e ;
               BSONObj obj ;
               e = it.next() ;
               PD_CHECK( Object == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Invalid argument[%s], rc:%d",
                         _argument.toString(false,false).c_str(), rc  ) ;
               obj = e.Obj() ;
               PD_CHECK( obj.hasField( FIELD_NAME_AUTOINC_FIELD ),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to get field [%s]", FIELD_NAME_AUTOINCREMENT );
               autoIncField = SDB_OSS_NEW rtnCLAutoincFieldArgument( obj ) ;
               PD_CHECK( NULL != autoIncField, SDB_OOM, error, PDERROR,
                         "Failed to allocate autoincrement field argument" ) ;
               rc = autoIncField->parseArgument() ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to parse autoincrement argument, rc: %d",
                            rc ) ;

               _autoIncFieldList.push_back( autoIncField ) ;
               autoIncField = NULL;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid field argument[%s], rc: %d",
                        _argument.toString( false, false ).c_str(), rc ) ;
         }
         parsedArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_CONTEXTLOCK |
                   RTN_ALTER_TASK_FLAG_SEQUENCE |
                   RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ;
      }

      if ( _argument.hasField( FIELD_NAME_DOMAIN_AUTO_REBALANCE ) )
      {
         argElement = _argument.getField( FIELD_NAME_DOMAIN_AUTO_REBALANCE ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]",
                   FIELD_NAME_DOMAIN_AUTO_REBALANCE ) ;
         _autoRebalance = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_AUTOREBALANCE_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_AUTO_INDEX_ID ) )
      {
         argElement = _argument.getField( FIELD_NAME_AUTO_INDEX_ID ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_AUTO_INDEX_ID ) ;
         _autoIndexID = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_AUTOIDXID_FIELD ) ;
         if ( !_autoIndexID )
         {
            setFlags( RTN_ALTER_TASK_TRANS_LOCK ) ;
         }
      }

      if ( _argument.hasField( FIELD_NAME_W ) )
      {
         argElement = _argument.getField( FIELD_NAME_W ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_W ) ;
         _replSize = argElement.numberInt() ;
         if ( _replSize == 0 )
         {
            _replSize = CLS_REPLSET_MAX_NODE_SIZE ;
         }
         PD_CHECK( -1 <= _replSize && _replSize <= CLS_REPLSET_MAX_NODE_SIZE,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]: invalid repl size [%d]",
                   FIELD_NAME_W, _replSize ) ;
         parsedArgumentMask( UTIL_CL_REPLSIZE_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_CONSISTENCY_STRATEGY ) )
      {
         UINT32 consistencyStrategy = 0 ;
         argElement = _argument.getField( FIELD_NAME_CONSISTENCY_STRATEGY ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_CONSISTENCY_STRATEGY ) ;
         consistencyStrategy = argElement.numberInt() ;
         PD_LOG_MSG_CHECK ( SDB_CONSISTENCY_NODE <= consistencyStrategy &&
                            SDB_CONSISTENCY_PRY_LOC_MAJOR >=
                            consistencyStrategy, SDB_INVALIDARG, error, PDERROR,
                            "Failed to get field [%s]: invalid consistency strategy"
                            " [%d]",FIELD_NAME_CONSISTENCY_STRATEGY,
                            consistencyStrategy ) ;
         _consistencyStrategy =
               (SDB_CONSISTENCY_STRATEGY)consistencyStrategy ;
         parsedArgumentMask( UTIL_CL_CONSISTENCYSTRATEGY_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_STRICTDATAMODE ) )
      {
         argElement = _argument.getField( FIELD_NAME_STRICTDATAMODE ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_STRICTDATAMODE ) ;
         _strictDataMode = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_STRICTDATAMODE_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_3PHASE ) ;
      }

      if ( _argument.hasField( FIELD_NAME_NOTRANS ) )
      {
         argElement = _argument.getField( FIELD_NAME_NOTRANS ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_NOTRANS ) ;
         _noTrans = argElement.boolean() ;
         parsedArgumentMask( UTIL_CL_NOTRANS_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_3PHASE |
                   RTN_ALTER_TASK_TRANS_LOCK ) ;
      }

      // Special non supported cases
      PD_CHECK( !_argument.hasField( FIELD_NAME_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_CAPPED ) ;

      PD_CHECK( !_argument.hasField( FIELD_NAME_ISMAINCL ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_ISMAINCL ) ;

      PD_CHECK( !_argument.hasField( FIELD_NAME_DOMAIN_AUTO_REBALANCE ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_DOMAIN_AUTO_REBALANCE ) ;

      PD_CHECK( !_argument.hasField( FIELD_NAME_NAME ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_NAME ) ;

      PD_CHECK( _conflictCheck(), SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Options conflict in alter option" ) ;

      if ( pmdGetDBRole() == SDB_ROLE_STANDALONE &&
           _argument.hasField( FIELD_NAME_AUTOINCREMENT ) ) {
        PD_LOG( PDERROR, "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_AUTOINCREMENT ) ;
        rc = SDB_OPTION_NOT_SUPPORT ;
        goto error ;
      }

      // ReplSize, ShardingKey, Compressed, CompressType, StrictDataMode,
      // NoTrans is allowed in main-collection, or checked by the altering
      // field itself
      if ( !testArgumentMask( ~( UTIL_CL_REPLSIZE_FIELD |
                                 UTIL_CL_SHDKEY_FIELD |
                                 UTIL_CL_COMPRESSED_FIELD |
                                 UTIL_CL_COMPRESSTYPE_FIELD |
                                 UTIL_CL_STRICTDATAMODE_FIELD |
                                 UTIL_CL_NOTRANS_FIELD ) ) )
      {
         setFlags( RTN_ALTER_TASK_FLAG_MAINCLALLOW ) ;
      }

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLSETATTRTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( autoIncField );
      goto done ;
   }

   BOOLEAN _rtnCLSetAttributeTask::_conflictCheck()
   {
      BOOLEAN pass = TRUE ;

      if ( _shardingArgument.isAutoSplit() && !_autoIndexID )
      {
         pass = FALSE ;
      }

      return pass ;
   }

   /*
      _rtnAlterCSTask implement
    */
   _rtnAlterCSTask::_rtnAlterCSTask ( const rtnAlterTaskSchema & schema,
                                      const BSONObj & argument )
   : _rtnAlterTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_COLLECTION_SPACE == schema.getObjectType(),
                  "schema is invalid" ) ;
   }

   _rtnAlterCSTask::~_rtnAlterCSTask ()
   {
   }

   /*
      _rtnCSSetDomainTask define
    */
   _rtnCSSetDomainTask::_rtnCSSetDomainTask ( const rtnAlterTaskSchema & schema,
                                              const BSONObj & argument )
   : _rtnAlterCSTask( schema, argument ),
     _domain( NULL )
   {
      SDB_ASSERT( RTN_ALTER_CS_SET_DOMAIN == schema.getActionType() ||
                  RTN_ALTER_CS_SET_ATTRIBUTES == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCSSetDomainTask::~_rtnCSSetDomainTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSSETDOMAINTASK_PARSEARG, "_rtnCSSetDomainTask::parseArgument" )
   INT32 _rtnCSSetDomainTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSSETDOMAINTASK_PARSEARG ) ;

      rc = _parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument, rc: %d", rc ) ;

      PD_CHECK( testArgumentMask( UTIL_CS_DOMAIN_FIELD ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: field [%s] is missing",
                FIELD_NAME_DOMAIN ) ;

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSSETDOMAINTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSSETDOMAINTASK__PARSEARG, "_rtnCSSetDomainTask::_parseArgument" )
   INT32 _rtnCSSetDomainTask::_parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSSETDOMAINTASK__PARSEARG ) ;

      BSONElement argElement ;

      if ( _argument.hasField( FIELD_NAME_DOMAIN ) )
      {
         argElement = _argument.getField( FIELD_NAME_DOMAIN ) ;
         PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_DOMAIN ) ;
         _domain = argElement.valuestr() ;
         parsedArgumentMask( UTIL_CS_DOMAIN_FIELD ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSSETDOMAINTASK__PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnCSRemoveDomainTask define
    */
   _rtnCSRemoveDomainTask::_rtnCSRemoveDomainTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnAlterCSTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CS_REMOVE_DOMAIN == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCSRemoveDomainTask::~_rtnCSRemoveDomainTask ()
   {
   }

   /*
      _rtnCSEnableCappedTask define
    */
   _rtnCSEnableCappedTask::_rtnCSEnableCappedTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnAlterCSTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CS_ENABLE_CAPPED == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCSEnableCappedTask::~_rtnCSEnableCappedTask ()
   {
   }

   /*
      _rtnCSDisableCappedTask define
    */
   _rtnCSDisableCappedTask::_rtnCSDisableCappedTask ( const rtnAlterTaskSchema & schema,
                                                      const BSONObj & argument )
   : _rtnAlterCSTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_CS_DISABLE_CAPPED == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCSDisableCappedTask::~_rtnCSDisableCappedTask ()
   {
   }

   /*
      _rtnCSSetAttributeTask implement
    */
   _rtnCSSetAttributeTask::_rtnCSSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnCSSetDomainTask( schema, argument ),
     _pageSize( DMS_PAGE_SIZE_DFT ),
     _lobPageSize( DMS_DEFAULT_LOB_PAGE_SZ ),
     _capped( FALSE )
   {
      SDB_ASSERT( RTN_ALTER_CS_SET_ATTRIBUTES == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnCSSetAttributeTask::~_rtnCSSetAttributeTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSSETATTRTASK_PARSEARG, "_rtnCSSetAttributeTask::parseArgument" )
   INT32 _rtnCSSetAttributeTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSSETATTRTASK_PARSEARG ) ;

      BSONElement argElement ;

      rc = _rtnCSSetDomainTask::_parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument, rc: %d", rc ) ;

      if ( testArgumentMask( UTIL_CS_DOMAIN_FIELD ) )
      {
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_PAGE_SIZE ) )
      {
         argElement = _argument.getField( FIELD_NAME_PAGE_SIZE ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_PAGE_SIZE ) ;
         if ( 0 != argElement.numberInt() )
         {
            // If value is 0, using the default value
            _pageSize = argElement.numberInt() ;
         }
         PD_CHECK ( _pageSize == DMS_PAGE_SIZE4K ||
                    _pageSize == DMS_PAGE_SIZE8K ||
                    _pageSize == DMS_PAGE_SIZE16K ||
                    _pageSize == DMS_PAGE_SIZE32K ||
                    _pageSize == DMS_PAGE_SIZE64K,
                    SDB_INVALIDARG, error, PDERROR,
                    "Failed to get field [%s]: %s must be 4K/8K/16K/32K/64K",
                    FIELD_NAME_PAGE_SIZE, FIELD_NAME_PAGE_SIZE ) ;
         parsedArgumentMask( UTIL_CS_PAGESIZE_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      if ( _argument.hasField( FIELD_NAME_LOB_PAGE_SIZE ) )
      {
         argElement = _argument.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
         PD_CHECK( NumberInt == argElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_LOB_PAGE_SIZE ) ;
         if ( 0 != argElement.numberInt() )
         {
            // If value is 0, using the default value
            _lobPageSize = argElement.numberInt() ;
         }
         PD_CHECK ( _lobPageSize == DMS_PAGE_SIZE4K ||
                    _lobPageSize == DMS_PAGE_SIZE8K ||
                    _lobPageSize == DMS_PAGE_SIZE16K ||
                    _lobPageSize == DMS_PAGE_SIZE32K ||
                    _lobPageSize == DMS_PAGE_SIZE64K ||
                    _lobPageSize == DMS_PAGE_SIZE128K ||
                    _lobPageSize == DMS_PAGE_SIZE256K ||
                    _lobPageSize == DMS_PAGE_SIZE512K,
                    SDB_INVALIDARG, error, PDERROR, "Failed to get field "
                    "[%s]: %s must be 4K/8K/16K/32K/64K/""128K/256K/512K",
                    FIELD_NAME_LOB_PAGE_SIZE, FIELD_NAME_LOB_PAGE_SIZE ) ;
         parsedArgumentMask( UTIL_CS_LOBPAGESIZE_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_CAPPED ) )
      {
         argElement = _argument.getField( FIELD_NAME_CAPPED ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_CAPPED ) ;
         _capped = argElement.boolean() ;
         parsedArgumentMask( UTIL_CS_CAPPED_FIELD ) ;
         setFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) ;
      }

      // Special non supported cases
      PD_CHECK( !_argument.hasField( FIELD_NAME_NAME ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to get field [%s]: it is not supported yet",
                FIELD_NAME_NAME ) ;

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSSETATTRTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnAlterDomainTask implement
    */
   _rtnAlterDomainTask::_rtnAlterDomainTask ( const rtnAlterTaskSchema & schema,
                                              const BSONObj & argument )
   : _rtnAlterTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN == schema.getObjectType(),
                  "schema is invalid" ) ;
   }

   _rtnAlterDomainTask::~_rtnAlterDomainTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERDOMAINTASK_PARSEARG, "_rtnAlterDomainTask::parseArgument" )
   INT32 _rtnAlterDomainTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERDOMAINTASK_PARSEARG ) ;

      BSONElement argElement ;

      if ( _argument.hasField( FIELD_NAME_GROUPS ) )
      {
         argElement = _argument.getField( FIELD_NAME_GROUPS ) ;
         if ( String == argElement.type() )
         {
            _groups.push_back( argElement.valuestr() ) ;
         }
         else if ( Array == argElement.type() )
         {
            BSONObjIterator iter( argElement.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement groupElement = iter.next() ;
               PD_CHECK( String == groupElement.type(), SDB_INVALIDARG, error,
                         PDERROR, "Failed to get field [%s]: invalid type",
                         FIELD_NAME_GROUPS ) ;
               _groups.push_back( groupElement.valuestr() ) ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field [%s]: invalid type",
                    FIELD_NAME_GROUPS ) ;
            goto error ;
         }
         parsedArgumentMask( UTIL_DOMAIN_GROUPS_FIELD ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERDOMAINTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainAddGroupTask implement
    */
   _rtnDomainAddGroupTask::_rtnDomainAddGroupTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_ADD_GROUPS == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainAddGroupTask::~_rtnDomainAddGroupTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINADDGRPTASK_PARSEARG, "_rtnDomainAddGroupTask::parseArgument" )
   INT32 _rtnDomainAddGroupTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINADDGRPTASK_PARSEARG ) ;

      rc = _rtnAlterDomainTask::parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument for task [%s]",
                   getActionName() ) ;

      PD_CHECK( !_groups.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument for task [%s]: no group is found",
                getActionName() ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINADDGRPTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainRemoveGroupTask implement
    */
   _rtnDomainRemoveGroupTask::_rtnDomainRemoveGroupTask ( const rtnAlterTaskSchema & schema,
                                                          const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_REMOVE_GROUPS == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainRemoveGroupTask::~_rtnDomainRemoveGroupTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINRMGRPTASK_PARSEARG, "_rtnDomainRemoveGroupTask::parseArgument" )
   INT32 _rtnDomainRemoveGroupTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINRMGRPTASK_PARSEARG ) ;

      rc = _rtnAlterDomainTask::parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument for task [%s]",
                   getActionName() ) ;

      PD_CHECK( !_groups.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument for task [%s]: no group is found",
                getActionName() ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINRMGRPTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainSetGroupTask implement
    */
   _rtnDomainSetGroupTask::_rtnDomainSetGroupTask ( const rtnAlterTaskSchema & schema,
                                                    const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_SET_GROUPS == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainSetGroupTask::~_rtnDomainSetGroupTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINSETRGTASK_PARSEARG, "_rtnDomainSetGroupTask::parseArgument" )
   INT32 _rtnDomainSetGroupTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINSETRGTASK_PARSEARG ) ;

      rc = _rtnAlterDomainTask::parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument for task [%s]",
                   getActionName() ) ;

      PD_CHECK( !_groups.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument for task [%s]: no group is found",
                getActionName() ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINSETRGTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainSetActiveLocationTask implement
    */
   _rtnDomainSetActiveLocationTask::_rtnDomainSetActiveLocationTask ( const rtnAlterTaskSchema & schema,
                                                              const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument ), _pLocation( NULL )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainSetActiveLocationTask::~_rtnDomainSetActiveLocationTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINSETACTIVELOCATIONTASK_PARSEARG, "_rtnDomainSetActiveLocationTask::parseArgument" )
   INT32 _rtnDomainSetActiveLocationTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINSETACTIVELOCATIONTASK_PARSEARG ) ;

      try
      {
         BSONElement optionEle ;

         // Get new ActiveLocation, this field should not be empty
         optionEle = _argument.getField( CAT_ACTIVE_LOCATION_NAME ) ;
         if ( optionEle.eoo() || String != optionEle.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
            goto error ;
         }
         // optionEle.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < optionEle.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         _pLocation = optionEle.valuestrsafe() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse arguments for "
                 "setActiveLocation task[%s], exception=%s",
                 _argument.toPoolString().c_str(), e.what() ) ;
         goto error ;
      }

      parsedArgumentMask( UTIL_DOMAIN_ACTIVE_LOCATION_FIELD ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINSETACTIVELOCATIONTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainSetLocationTask implement
    */
   _rtnDomainSetLocationTask::_rtnDomainSetLocationTask ( const rtnAlterTaskSchema & schema,
                                                          const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument ), _pHostName( NULL ), _pLocation( NULL )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_SET_LOCATION == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainSetLocationTask::~_rtnDomainSetLocationTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINSETLOCATIONTASK_PARSEARG, "_rtnDomainSetLocationTask::parseArgument" )
   INT32 _rtnDomainSetLocationTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINSETLOCATIONTASK_PARSEARG ) ;

      try
      {
         BSONElement optionEle ;

         // Get host name, this field should not be empty
         optionEle = _argument.getField( FIELD_NAME_HOST ) ;
         if ( optionEle.eoo() || String != optionEle.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", FIELD_NAME_HOST ) ;
            goto error ;
         }
         _pHostName = optionEle.valuestrsafe() ;

         // Get new Location, this field should not be empty
         optionEle = _argument.getField( FIELD_NAME_LOCATION ) ;
         if ( optionEle.eoo() || String != optionEle.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", FIELD_NAME_LOCATION ) ;
            goto error ;
         }
         // optionEle.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < optionEle.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         _pLocation = optionEle.valuestrsafe() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse arguments for "
                 "setLocation task[%s], exception=%s",
                 _argument.toPoolString().c_str(), e.what() ) ;
         goto error ;
      }

      parsedArgumentMask( UTIL_DOMAIN_LOCATION_FIELD ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINSETLOCATIONTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnDomainSetAttributeTask implement
    */
   _rtnDomainSetAttributeTask::_rtnDomainSetAttributeTask ()
   : _rtnAlterDomainTask( rtnGetSetAttrTask( RTN_ALTER_DOMAIN ), BSONObj() )
   {
   }

   _rtnDomainSetAttributeTask::_rtnDomainSetAttributeTask ( const rtnAlterTaskSchema & schema,
                                                            const BSONObj & argument )
   : _rtnAlterDomainTask( schema, argument ),
     _autoSplit( FALSE ),
     _autoRebalance( FALSE )
   {
      SDB_ASSERT( RTN_ALTER_DOMAIN_SET_ATTRIBUTES == schema.getActionType(),
                  "schema is invalid" ) ;
   }

   _rtnDomainSetAttributeTask::~_rtnDomainSetAttributeTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDOMAINSETATTRTASK_PARSEARG, "_rtnDomainSetAttributeTask::parseArgument" )
   INT32 _rtnDomainSetAttributeTask::parseArgument ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDOMAINSETATTRTASK_PARSEARG ) ;

      BSONElement argElement ;

      rc = _rtnAlterDomainTask::parseArgument() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse argument for task [%s]",
                   getActionName() ) ;

      if ( _argument.hasField( FIELD_NAME_DOMAIN_AUTO_SPLIT ) )
      {
         argElement = _argument.getField( FIELD_NAME_DOMAIN_AUTO_SPLIT ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_DOMAIN_AUTO_SPLIT ) ;
         _autoSplit = argElement.boolean() ;
         parsedArgumentMask( UTIL_DOMAIN_AUTOSPLIT_FIELD ) ;
      }

      if ( _argument.hasField( FIELD_NAME_DOMAIN_AUTO_REBALANCE ) )
      {
         argElement = _argument.getField( FIELD_NAME_DOMAIN_AUTO_REBALANCE ) ;
         PD_CHECK( Bool == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s]",
                   FIELD_NAME_DOMAIN_AUTO_REBALANCE ) ;
         _autoRebalance = argElement.boolean() ;
         parsedArgumentMask( UTIL_DOMAIN_AUTOREBALANCE_FIELD ) ;
      }

      PD_CHECK( _argumentCount == (UINT32)_argument.nFields(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse argument: contain unknown fields" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDOMAINSETATTRTASK_PARSEARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
