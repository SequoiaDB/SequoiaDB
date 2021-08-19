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

   Source File Name = rtnCommand.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCommand.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnContextDel.hpp"
#include "rtnContext.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "monDump.hpp"
#include "ossMem.h"
#include "rtnContextListLob.hpp"
#include "rtnSessionProperty.hpp"
#include "rtnAlter.hpp"
#include "aggrDef.hpp"
#include "utilCompressor.hpp"
#include "msgMessageFormat.hpp"

#if defined (_DEBUG)
// for qgmDebugQuery function
#endif

using namespace bson ;
using namespace std ;

// Default size of capped collection for text index. The unit is MB. So its 30G.
#define TEXT_INDEX_DATA_BUFF_DEFAULT_SIZE  ( 30 * 1024 )

namespace engine
{
   _rtnCommand::_rtnCommand ()
   {
      _fromService = 0 ;
   }

   _rtnCommand::~_rtnCommand ()
   {
   }

   void _rtnCommand::setFromService( INT32 fromService )
   {
      _fromService = fromService ;
   }

   BOOLEAN _rtnCommand::hasBuff() const
   {
      return _buff.size() > 0 ? TRUE : FALSE ;
   }

   const rtnContextBuf& _rtnCommand::getBuff() const
   {
      return _buff ;
   }

   INT32 _rtnCommand::spaceNode()
   {
      return CMD_SPACE_NODE_ALL ;
   }

   INT32 _rtnCommand::spaceService ()
   {
      return CMD_SPACE_SERVICE_ALL ;
   }

   BOOLEAN _rtnCommand::writable ()
   {
      return FALSE ;
   }

   const CHAR *_rtnCommand::collectionFullName ()
   {
      return NULL ;
   }

   const CHAR *_rtnCommand::spaceName()
   {
      return NULL ;
   }

   _rtnCmdBuilder::_rtnCmdBuilder ()
   {
      _pCmdInfoRoot = NULL ;
   }

   _rtnCmdBuilder::~_rtnCmdBuilder ()
   {
      if ( _pCmdInfoRoot )
      {
         _releaseCmdInfo ( _pCmdInfoRoot ) ;
         _pCmdInfoRoot = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER_RELCMDINFO, "_rtnCmdBuilder::_releaseCmdInfo" )
   void _rtnCmdBuilder::_releaseCmdInfo ( _cmdBuilderInfo *pCmdInfo )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER_RELCMDINFO ) ;
      if ( pCmdInfo->next )
      {
         _releaseCmdInfo ( pCmdInfo->next ) ;
         pCmdInfo->next = NULL ;
      }
      if ( pCmdInfo->sub )
      {
         _releaseCmdInfo ( pCmdInfo->sub ) ;
         pCmdInfo->sub = NULL ;
      }

      SDB_OSS_DEL pCmdInfo ;
      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER_RELCMDINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__REGISTER, "_rtnCmdBuilder::_register" )
   INT32 _rtnCmdBuilder::_register ( const CHAR * name, CDM_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__REGISTER ) ;

      if ( !_pCmdInfoRoot )
      {
         _pCmdInfoRoot = SDB_OSS_NEW _cmdBuilderInfo ;
         _pCmdInfoRoot->cmdName = name ;
         _pCmdInfoRoot->createFunc = pFunc ;
         _pCmdInfoRoot->nameSize = ossStrlen ( name ) ;
         _pCmdInfoRoot->next = NULL ;
         _pCmdInfoRoot->sub = NULL ;
      }
      else
      {
         rc = _insert ( _pCmdInfoRoot, name, pFunc ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Register command [%s] failed", name ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCMDBUILDER__REGISTER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__INSERT, "_rtnCmdBuilder::_insert" )
   INT32 _rtnCmdBuilder::_insert ( _cmdBuilderInfo * pCmdInfo,
                                   const CHAR * name, CDM_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__INSERT ) ;
      _cmdBuilderInfo *newCmdInfo = NULL ;
      UINT32 sameNum = _near ( pCmdInfo->cmdName.c_str(), name ) ;

      if ( sameNum == 0 )
      {
         if ( !pCmdInfo->sub )
         {
            newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
            newCmdInfo->cmdName = name ;
            newCmdInfo->createFunc = pFunc ;
            newCmdInfo->nameSize = ossStrlen(name) ;
            newCmdInfo->next = NULL ;
            newCmdInfo->sub = NULL ;

            pCmdInfo->sub = newCmdInfo ;
         }
         else
         {
            rc = _insert ( pCmdInfo->sub, name, pFunc ) ;
         }
      }
      else if ( sameNum == pCmdInfo->nameSize )
      {
         if ( sameNum == ossStrlen ( name ) )
         {
            if ( !pCmdInfo->createFunc )
            {
               pCmdInfo->createFunc = pFunc ;
            }
            else
            {
               rc = SDB_SYS ; //already exist
            }
         }
         else if ( !pCmdInfo->next )
         {
            newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
            newCmdInfo->cmdName = &name[sameNum] ;
            newCmdInfo->createFunc = pFunc ;
            newCmdInfo->nameSize = ossStrlen(&name[sameNum]) ;
            newCmdInfo->next = NULL ;
            newCmdInfo->sub = NULL ;

            pCmdInfo->next = newCmdInfo ;
         }
         else
         {
            rc = _insert ( pCmdInfo->next, &name[sameNum], pFunc ) ;
         }
      }
      else
      {
         //split next node first
         newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
         newCmdInfo->cmdName = pCmdInfo->cmdName.substr( sameNum ) ;
         newCmdInfo->createFunc = pCmdInfo->createFunc ;
         newCmdInfo->nameSize = pCmdInfo->nameSize - sameNum ;
         newCmdInfo->next = pCmdInfo->next ;
         newCmdInfo->sub = NULL ;

         pCmdInfo->next = newCmdInfo ;

         //change cur node
         pCmdInfo->cmdName = pCmdInfo->cmdName.substr ( 0, sameNum ) ;
         pCmdInfo->nameSize = sameNum ;

         if ( sameNum != ossStrlen ( name ) )
         {
            pCmdInfo->createFunc = NULL ;

            rc = _insert ( newCmdInfo, &name[sameNum], pFunc ) ;
         }
         else
         {
            pCmdInfo->createFunc = pFunc ;
         }
      }

      PD_TRACE_EXITRC ( SDB__RTNCMDBUILDER__INSERT, rc ) ;
      return rc ;
   }

   _rtnCommand *_rtnCmdBuilder::create ( const CHAR *command )
   {
      CDM_NEW_FUNC pFunc = _find ( command ) ;
      if ( pFunc )
      {
         return (*pFunc)() ;
      }

      return NULL ;
   }

   void _rtnCmdBuilder::release ( _rtnCommand *pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__FIND, "_rtnCmdBuilder::_find" )
   CDM_NEW_FUNC _rtnCmdBuilder::_find ( const CHAR * name )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__FIND ) ;
      CDM_NEW_FUNC pFunc = NULL ;
      _cmdBuilderInfo *pCmdNode = _pCmdInfoRoot ;
      UINT32 index = 0 ;
      BOOLEAN findRoot = FALSE ;

      while ( pCmdNode )
      {
         findRoot = FALSE ;
         if ( pCmdNode->cmdName.at ( 0 ) != name[index] )
         {
            pCmdNode = pCmdNode->sub ;
         }
         else
         {
            findRoot = TRUE ;
         }

         if ( findRoot )
         {
            if ( _near ( pCmdNode->cmdName.c_str(), &name[index] )
               != pCmdNode->nameSize )
            {
               break ;
            }
            index += pCmdNode->nameSize ;
            if ( name[index] == 0 ) //find
            {
               pFunc = pCmdNode->createFunc ;
               break ;
            }

            pCmdNode = pCmdNode->next ;
         }
      }

      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER__FIND ) ;
      return pFunc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__NEAR, "_rtnCmdBuilder::_near" )
   UINT32 _rtnCmdBuilder::_near ( const CHAR *str1, const CHAR *str2 )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDBUILDER__NEAR ) ;
      UINT32 same = 0 ;
      while ( str1[same] && str2[same] && str1[same] == str2[same] )
      {
         ++same ;
      }

      PD_TRACE_EXIT ( SDB__RTNCMDBUILDER__NEAR ) ;
      return same ;
   }

   _rtnCmdBuilder *getRtnCmdBuilder ()
   {
      static _rtnCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDASSIT__RTNCMDASSIT, "_rtnCmdAssit::_rtnCmdAssit" )
   _rtnCmdAssit::_rtnCmdAssit ( CDM_NEW_FUNC pFunc )
   {
      PD_TRACE_ENTRY ( SDB__RTNCMDASSIT__RTNCMDASSIT ) ;
      if ( pFunc )
      {
         _rtnCommand *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getRtnCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
      PD_TRACE_EXIT ( SDB__RTNCMDASSIT__RTNCMDASSIT ) ;
   }

   _rtnCmdAssit::~_rtnCmdAssit ()
   {
   }

   //Command list:
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnUpdateNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnActiveGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnStartNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdownNode)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdownGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetConfig)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListDomains)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListGroups)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListProcedures)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateProcedure)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveProcedure)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCSInDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCLInDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCataGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnAlterDomain)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnAddDomainGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveDomainGroup)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCata )
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCataIntr)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnWaitTask)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTask)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListUsers)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetDCInfo)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSequences)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSequencesIntr)
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSequences)

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnBackup)
   _rtnBackup::_rtnBackup ()
      :_matherBuff ( NULL )
   {
      _backupName       = NULL ;
      _path             = NULL ;
      _desp             = NULL ;
      _ensureInc        = FALSE ;
      _rewrite          = FALSE ;
   }

   _rtnBackup::~_rtnBackup ()
   {
   }

   const CHAR *_rtnBackup::name ()
   {
      return NAME_BACKUP_OFFLINE ;
   }

   RTN_COMMAND_TYPE _rtnBackup::type ()
   {
      return CMD_BACKUP_OFFLINE ;
   }

   INT32 _rtnBackup::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                            const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                            const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      _matherBuff = pMatcherBuff ;

      if ( !_matherBuff )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj matcher( _matherBuff ) ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_NAME, &_backupName ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_NAME,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_PATH, &_path ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_PATH,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetStringElement( matcher, FIELD_NAME_DESP, &_desp ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d", FIELD_NAME_DESP,
                   rc ) ;
         rc = SDB_OK ;

         rc = rtnGetBooleanElement( matcher, FIELD_NAME_ENSURE_INC,
                                    _ensureInc ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d",
                   FIELD_NAME_ENSURE_INC, rc ) ;
         rc = SDB_OK ;

         rc = rtnGetBooleanElement( matcher, FIELD_NAME_OVERWRITE, _rewrite ) ;
         PD_CHECK( SDB_OK == rc || SDB_FIELD_NOT_EXIST == rc, rc, error,
                   PDWARNING, "Get field[%s] failed, rc: %d",
                   FIELD_NAME_OVERWRITE, rc ) ;
         rc = SDB_OK ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnBackup::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                            SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                            INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( _matherBuff ) ;
         rc = rtnBackup( cb, _path, _backupName, _ensureInc, _rewrite,
                         _desp, matcher ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what () ) ;
         rc = SDB_SYS ;
      }
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCollection)
   _rtnCreateCollection::_rtnCreateCollection ()
   :_collectionName ( NULL ),
    _attributes( 0 ),
    _clUniqueID( UTIL_CLUNIQUEID_LOCAL ),
    _compressorType( UTIL_COMPRESSOR_INVALID )
   {
   }

   _rtnCreateCollection::~_rtnCreateCollection ()
   {
   }

   const CHAR *_rtnCreateCollection::name()
   {
      return NAME_CREATE_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnCreateCollection::type()
   {
      return CMD_CREATE_COLLECTION ;
   }

   BOOLEAN _rtnCreateCollection::writable()
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_INIT, "_rtnCreateCollection::init" )
   INT32 _rtnCreateCollection::init( INT32 flags, INT64 numToSkip,
                                     INT64 numToReturn,
                                     const CHAR * pMatcherBuff,
                                     const CHAR * pSelectBuff,
                                     const CHAR * pOrderByBuff,
                                     const CHAR * pHintBuff)
   {
      INT32 rc = SDB_OK ;
      BOOLEAN enSureShardIdx = TRUE ;
      BOOLEAN isCompressed = FALSE ;
      BOOLEAN hasCompressed = TRUE ;
      BOOLEAN hasCompressType = TRUE ;
      BOOLEAN strictDataMode = FALSE ;
      BOOLEAN autoIndexId = TRUE ;
      BOOLEAN capped = FALSE ;
      const CHAR *compressionType = NULL ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECL_INIT ) ;
      BSONObj matcher ( pMatcherBuff ) ;
      BSONElement ele ;

      rc = rtnGetStringElement ( matcher, FIELD_NAME_NAME,
                                 &_collectionName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to extract %s field for collection "
                  "creation, rc = %d", FIELD_NAME_NAME, rc ) ;
         goto error ;
      }

      // capped, should be extracted before EnsureShardingKey and AutoIndexId
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_CAPPED, capped ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         capped = FALSE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Field[%s] value error in obj[%s]",
                   FIELD_NAME_CAPPED, matcher.toString().c_str() ) ;

      // ensure sharding key
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_ENSURE_SHDINDEX,
                                 enSureShardIdx ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         if ( capped )
         {
            enSureShardIdx = FALSE ;
         }
         else
         {
            enSureShardIdx = TRUE ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Field[%s] value is error in obj[%s]",
                   FIELD_NAME_ENSURE_SHDINDEX, matcher.toString().c_str() ) ;
      // if we want to create sharding key index, let's do it
      if ( enSureShardIdx )
      {
         rc = rtnGetObjElement ( matcher, FIELD_NAME_SHARDINGKEY,
                                 _shardingKey ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Field[%s] value is error in obj[%s]",
                      FIELD_NAME_SHARDINGKEY,
                      matcher.toString().c_str() ) ;
      }

      // check compress
      rc = rtnGetBooleanElement ( matcher, FIELD_NAME_COMPRESSED,
                                  isCompressed ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         hasCompressed = FALSE ;
      }
      rc = rtnGetStringElement( matcher, FIELD_NAME_COMPRESSIONTYPE,
                                &compressionType ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         hasCompressType = FALSE ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get CompressionType, rc: %d", rc ) ;
         goto error ;
      }
      else
      {
         if ( 0 == ossStrcmp( compressionType, VALUE_NAME_LZW ) )
         {
            _compressorType = UTIL_COMPRESSOR_LZW ;
         }
         else if ( 0 == ossStrcmp( compressionType, VALUE_NAME_SNAPPY ) )
         {
            _compressorType = UTIL_COMPRESSOR_SNAPPY ;
         }
         else
         {
            PD_LOG( PDERROR, "Compression type[%s] is invalid",
                    compressionType ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( isCompressed && !hasCompressType )
      {
         _compressorType = UTIL_COMPRESSOR_LZW ;
      }
      if ( !hasCompressed && hasCompressType )
      {
         isCompressed = TRUE ;
      }
      if ( !isCompressed && hasCompressType )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "CompressionType can't be set when Compressed is false" ) ;
         goto error ;
      }
      if ( !capped && !hasCompressed && !hasCompressType )
      {
         isCompressed = TRUE ;
         _compressorType = UTIL_COMPRESSOR_LZW ;
      }
      if ( isCompressed )
      {
         _attributes |= DMS_MB_ATTR_COMPRESSED ;
      }

      // check strictDataMode
      rtnGetBooleanElement ( matcher, FIELD_NAME_STRICTDATAMODE,
                             strictDataMode ) ;
      if ( strictDataMode )
      {
         _attributes |= DMS_MB_ATTR_STRICTDATAMODE ;
      }

      /// auto index id
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_AUTO_INDEX_ID,
                                 autoIndexId ) ;
      if ( SDB_OK == rc && !autoIndexId )
      {
         _attributes |= DMS_MB_ATTR_NOIDINDEX ;
      }
      if ( SDB_FIELD_NOT_EXIST == rc && capped )
      {
         autoIndexId = FALSE ;
         _attributes |= DMS_MB_ATTR_NOIDINDEX ;
      }

      if ( capped )
      {
         INT64 maxSize = 0 ;
         INT64 maxRecNum = 0 ;
         BOOLEAN overwrite = FALSE ;
         BSONObjBuilder builder ;

         if ( isCompressed )
         {
            PD_LOG( PDWARNING,
                    "Compression is not allowed on capped collection." ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( ( !_shardingKey.isEmpty() && enSureShardIdx ) ||
              autoIndexId )
         {
            PD_LOG( PDWARNING,
                    "Index is not allowed to be created on capped collection, "
                    "including $id index and $shard index.") ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _attributes |= DMS_MB_ATTR_CAPPED ;
         rc = rtnGetNumberLongElement( matcher, FIELD_NAME_SIZE, maxSize ) ;
         if ( rc )
         {
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               PD_LOG( PDERROR, "Field[%s] must always be used when Capped is "
                       "true in obj[%s]",
                       FIELD_NAME_SIZE, matcher.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
            }
            else
            {
               PD_LOG( PDERROR, "Field[%s] value is error in obj[%s]",
                       FIELD_NAME_SIZE, matcher.toString().c_str() ) ;
            }
            goto error ;
         }
         if ( maxSize <= 0 || maxSize > DMS_CAP_CL_SIZE )
         {
            PD_LOG( PDERROR, "Invalid Size[ %lld ] when creating capped "
                    "collection", maxSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         maxSize = ossRoundUpToMultipleX( maxSize << 20,
                                          DMS_MAX_CL_SIZE_ALIGN_SIZE ) ;
         builder.append( FIELD_NAME_SIZE, maxSize ) ;

         // Max/OverWrite is optional.
         rc = rtnGetNumberLongElement( matcher, FIELD_NAME_MAX, maxRecNum ) ;
         if ( SDB_OK != rc && SDB_FIELD_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "Field[%s] value is error in obj[%s]",
                    FIELD_NAME_MAX, matcher.toString().c_str() ) ;
            goto error ;
         }
         if ( maxRecNum < 0 )
         {
            PD_LOG( PDERROR, "Invalid Max[ %lld ] when creating capped "
                    "collection", maxRecNum ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.append( FIELD_NAME_MAX, maxRecNum ) ;

         rc = rtnGetBooleanElement( matcher, FIELD_NAME_OVERWRITE, overwrite ) ;
         if ( SDB_OK != rc && SDB_FIELD_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "Field[%s] value is error in obj[%s]",
                    FIELD_NAME_OVERWRITE, matcher.toString().c_str() ) ;
            goto error ;
         }
         builder.appendBool( FIELD_NAME_OVERWRITE, overwrite ) ;

         _extOptions = builder.obj() ;
      }

      rc = SDB_OK ;
   done :
      PD_TRACE_EXITRC ( SDB__RTNCREATECL_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   const CHAR *_rtnCreateCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_DOIT, "_rtnCreateCollection::doit" )
   INT32 _rtnCreateCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                      INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECL_DOIT ) ;

      rc = rtnCreateCollectionCommand ( _collectionName, _shardingKey,
                                        _attributes, cb, dmsCB, dpsCB,
                                        _clUniqueID, _compressorType, 0, FALSE,
                                        ( _extOptions.isEmpty() ?
                                         NULL : &_extOptions ) ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         CHAR szTmp[ 50 ] = { 0 } ;
         mbAttr2String( _attributes, szTmp, sizeof(szTmp) - 1 ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc,
                           "ShardingKey:%s, Attribute:0x%08x(%s), "
                           "CompressType:%d(%s), extend options: %s",
                           _shardingKey.toString().c_str(),
                           _attributes, szTmp,
                           _compressorType,
                           utilCompressType2String( _compressorType ),
                           _extOptions.toString().c_str() ) ;
      }

      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCREATECL_DOIT, rc ) ;
      return rc ;
   error:
      _clean( cb, dmsCB, dpsCB ) ;
      goto done ;
   }

   void _rtnCreateCollection::setCLUniqueID( utilCLUniqueID clUniqueID )
   {
      _clUniqueID = clUniqueID ;
   }

   // Clean when error happened.
   // The main job here is to remove the collection space if this is the only
   // collection in the collection space.
   // Attention: This should only be done on data node in a cluster when the
   // command is from shard plane.

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL__CLEAN, "_rtnCreateCollection::_clean" )
   void _rtnCreateCollection::_clean( pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB__RTNCREATECL__CLEAN ) ;

      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         INT32 rc = SDB_OK ;
         string nameStr( _collectionName ) ;
         string csName = nameStr.substr( 0, nameStr.find( '.' ) ) ;

         rc = dmsCB->dropEmptyCollectionSpace( csName.c_str(), cb, dpsCB ) ;
         if ( rc && SDB_DMS_CS_NOT_EMPTY != rc )
         {
            // Just logging the error, but can do nothing about it.
            PD_LOG( PDWARNING, "Drop new created collection space[%s] failed, "
                    "rc: %d", csName.c_str(), rc ) ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNCREATECL__CLEAN ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCollectionspace)
   _rtnCreateCollectionspace::_rtnCreateCollectionspace ()
   :_spaceName( NULL ),
    _csUniqueID( UTIL_CSUNIQUEID_LOCAL ),
    _pageSize( 0 ),
    _lobPageSize( 0 ),
    _storageType( DMS_STORAGE_NORMAL )
   {

   }

   _rtnCreateCollectionspace::~_rtnCreateCollectionspace ()
   {
   }

   const CHAR *_rtnCreateCollectionspace::name ()
   {
      return NAME_CREATE_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnCreateCollectionspace::type ()
   {
      return CMD_CREATE_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnCreateCollectionspace::writable ()
   {
      return TRUE ;
   }

   const CHAR* _rtnCreateCollectionspace::spaceName()
   {
      return _spaceName ;
   }

   INT32 _rtnCreateCollectionspace::init ( INT32 flags, INT64 numToSkip,
                                           INT64 numToReturn,
                                           const CHAR * pMatcherBuff,
                                           const CHAR * pSelectBuff,
                                           const CHAR * pOrderByBuff,
                                           const CHAR * pHintBuff)
   {
      BSONObj mather ( pMatcherBuff ) ;
      INT32 rc = rtnGetIntElement ( mather, FIELD_NAME_PAGE_SIZE,
                                    _pageSize ) ;
      if ( SDB_OK != rc )
      {
         _pageSize = DMS_PAGE_SIZE_DFT ;
      }

      rc = rtnGetIntElement( mather, FIELD_NAME_LOB_PAGE_SIZE,
                             _lobPageSize ) ;
      if ( SDB_OK != rc )
      {
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      BOOLEAN capped = FALSE ;
      rc = rtnGetBooleanElement( mather, FIELD_NAME_CAPPED, capped ) ;
      if ( SDB_OK == rc && capped  )
      {
         _storageType = DMS_STORAGE_CAPPED ;
      }
      else
      {
         _storageType = DMS_STORAGE_NORMAL ;
      }

      return rtnGetStringElement ( mather, FIELD_NAME_NAME, &_spaceName ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECS_DOIT, "_rtnCreateCollectionspace::doit" )
   INT32 _rtnCreateCollectionspace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECS_DOIT ) ;

      rc = rtnCreateCollectionSpaceCommand ( _spaceName, cb, dmsCB,
                                             dpsCB, _csUniqueID, _pageSize,
                                             _lobPageSize, _storageType ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _spaceName, rc,
                           "PageSize:%u, LobPageSize:%u",
                           _pageSize, _lobPageSize ) ;
      }

      PD_TRACE_EXITRC ( SDB__RTNCREATECS_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateIndex)

   _rtnCreateIndex::_rtnCreateIndex ()
   : _collectionName ( NULL ),
     _sortBufferSize ( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ),
     _textIdx( FALSE )
   {
   }

   _rtnCreateIndex::~_rtnCreateIndex ()
   {
   }

   const CHAR *_rtnCreateIndex::name ()
   {
      return NAME_CREATE_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnCreateIndex::type ()
   {
      return CMD_CREATE_INDEX ;
   }

   BOOLEAN _rtnCreateIndex::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnCreateIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_INIT, "_rtnCreateIndex::init" )
   INT32 _rtnCreateIndex::init ( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      BSONObj hint ( pHintBuff ) ;
      BOOLEAN hasSortBufSz = FALSE ;

      INT32 rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                       &_collectionName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get string [collection] " ) ;
         goto error ;
      }

      rc = rtnGetObjElement ( arg, FIELD_NAME_INDEX, _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get object index " ) ;
         goto error ;
      }

      rc = rtnConvertIndexDef( _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to convert index definition" ) ;
         goto error ;
      }

      rc = _validateDef( _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to validate index definition: %s, rc: %d",
                  _index.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( arg.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         hasSortBufSz = TRUE ;
         rc = rtnGetIntElement( arg, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufferSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get index sort buffer, arg: %s",
                     arg.toString().c_str() ) ;
            goto error ;
         }
      }
      else if ( hint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         hasSortBufSz = TRUE ;
         rc = rtnGetIntElement( hint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufferSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get index sort buffer, hint: %s",
                     hint.toString().c_str() ) ;
            goto error ;
         }
      }
      if ( _sortBufferSize < 0 )
      {
         PD_LOG ( PDERROR, "invalid index sort buffer size: %d",
                  _sortBufferSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !hasSortBufSz )
      {
         // For text index, the "sort buffer size" is actually used as the 'Size'
         // option for the corresponding capped collection.
         if ( _textIdx )
         {
            _sortBufferSize = TEXT_INDEX_DATA_BUFF_DEFAULT_SIZE ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCREATEINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_DOIT, "_rtnCreateIndex::doit" )
   INT32 _rtnCreateIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_DOIT ) ;
      BOOLEAN isSys = FALSE ;

      // Currently only support text index in cluster.
      if ( _textIdx && ( CMD_SPACE_SERVICE_SHARD != getFromService() ) )
      {
         PD_LOG( PDERROR, "Text index is only supported in cluster" ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      if ( !pmdGetOptionCB()->authEnabled() )
      {
         isSys = TRUE ;
      }

      rc = rtnCreateIndexCommand ( _collectionName, _index, cb,
                                   dmsCB, dpsCB, isSys, _sortBufferSize,
                                   &_writeResult ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc,
                           "IndexDef:%s, SortBuffSize:%d",
                           _index.toString().c_str(), _sortBufferSize ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCREATEINDEX_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Check if there is mixed use of normal index and text index.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX__VALIDATEDEF, "_rtnCreateIndex::_validateDef" )
   INT32 _rtnCreateIndex::_validateDef( const BSONObj &index )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCREATEINDEX__VALIDATEDEF ) ;
      BOOLEAN hasText = FALSE ;
      const string textFieldVal = "text" ;
      BSONObj idxDef = index.getObjectField( IXM_FIELD_NAME_KEY ) ;
      BSONObjIterator itr( idxDef ) ;

      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;
         if ( ele.eoo() )
         {
            PD_LOG( PDERROR, "Index definition ended unexpected. "
                    "Definition: %s", idxDef.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( String == ele.type() && textFieldVal == ele.String() )
         {
            hasText = TRUE ;
         }
         else
         {
            if ( hasText )
            {
               PD_LOG( PDERROR, "Text index can only contain fields specified "
                       "as text. Definition: %s", idxDef.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }

      _textIdx = hasText ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCREATEINDEX__VALIDATEDEF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropCollection)

   _rtnDropCollection::_rtnDropCollection ()
   :_collectionName ( NULL )
   {
   }

   _rtnDropCollection::~_rtnDropCollection ()
   {
   }

   const CHAR *_rtnDropCollection::name ()
   {
      return NAME_DROP_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnDropCollection::type ()
   {
      return CMD_DROP_COLLECTION ;
   }

   BOOLEAN _rtnDropCollection::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnDropCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   INT32 _rtnDropCollection::init ( INT32 flags, INT64 numToSkip,
                                    INT64 numToReturn,
                                    const CHAR * pMatcherBuff,
                                    const CHAR * pSelectBuff,
                                    const CHAR * pOrderByBuff,
                                    const CHAR * pHintBuff )
   {
      BSONObj arg ( pMatcherBuff ) ;
      return rtnGetStringElement ( arg, FIELD_NAME_NAME, &_collectionName ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCL_DOIT, "_rtnDropCollection::doit" )
   INT32 _rtnDropCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                    SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                    INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPCL_DOIT ) ;
      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextDelCL *delContext = NULL;
         rc = rtnCB->contextNew( RTN_CONTEXT_DELCL, (rtnContext **)&delContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, drop "
                      "collection failed(rc=%d)", rc );
         rc = delContext->open( _collectionName, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop "
                      "collection failed(rc=%d)",
                      rc );
      }
      else
      {
         rc = rtnDropCollectionCommand ( _collectionName, cb, dmsCB, dpsCB ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNDROPCL_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb );
         *pContextID = -1;
      }
      goto done;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropCollectionspace)

   _rtnDropCollectionspace::_rtnDropCollectionspace ()
   :_spaceName ( NULL )
   {
   }

   _rtnDropCollectionspace::~_rtnDropCollectionspace ()
   {
   }

   const CHAR *_rtnDropCollectionspace::spaceName ()
   {
      return _spaceName ;
   }

   const CHAR *_rtnDropCollectionspace::name ()
   {
      return NAME_DROP_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnDropCollectionspace::type ()
   {
      return CMD_DROP_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnDropCollectionspace::writable ()
   {
      return TRUE ;
   }

   INT32 _rtnDropCollectionspace::init ( INT32 flags, INT64 numToSkip,
                                         INT64 numToReturn,
                                         const CHAR * pMatcherBuff,
                                         const CHAR * pSelectBuff,
                                         const CHAR * pOrderByBuff,
                                         const CHAR * pHintBuff )
   {
      BSONObj arg ( pMatcherBuff ) ;

      return rtnGetStringElement ( arg, FIELD_NAME_NAME, &_spaceName ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCS_DOIT, "_rtnDropCollectionspace::doit" )
   INT32 _rtnDropCollectionspace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                         SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                         INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPCS_DOIT ) ;
      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextDelCS *delContext = NULL;
         rc = rtnCB->contextNew( RTN_CONTEXT_DELCS,
                                 (rtnContext **)&delContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, "
                      "drop cs failed(rc=%d)", rc );
         rc = delContext->open( _spaceName, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, drop cs "
                      "failed(rc=%d)", rc );
      }
      else
      {
         rc = rtnDropCollectionSpaceCommand ( _spaceName, cb, dmsCB, dpsCB ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _spaceName, rc, "" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNDROPCS_DOIT, rc ) ;
      return rc ;
   error :
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb );
         *pContextID = -1;
      }
      goto done;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropIndex)

   _rtnDropIndex::_rtnDropIndex ()
   :_collectionName ( NULL )
   {
   }

   _rtnDropIndex::~_rtnDropIndex ()
   {
   }

   const CHAR *_rtnDropIndex::name ()
   {
      return NAME_DROP_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnDropIndex::type ()
   {
      return CMD_DROP_INDEX ;
   }

   const CHAR *_rtnDropIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   BOOLEAN _rtnDropIndex::writable ()
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_INIT, "_rtnDropIndex::init" )
   INT32 _rtnDropIndex::init ( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR * pMatcherBuff,
                               const CHAR * pSelectBuff,
                               const CHAR * pOrderByBuff,
                               const CHAR * pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__RTNDROPINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      INT32 rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                       &_collectionName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get string[collection]" ) ;
         goto error ;
      }

      rc = rtnGetObjElement ( arg, FIELD_NAME_INDEX, _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get index object " ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__RTNDROPINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_DOIT, "_rtnDropIndex::doit" )
   INT32 _rtnDropIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                               SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                               INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDROPINDEX_DOIT ) ;
      BOOLEAN isSys = FALSE ;
      BSONElement ele = _index.firstElement() ;

      if ( !pmdGetOptionCB()->authEnabled() )
      {
         isSys = TRUE ;
      }
      rc = rtnDropIndexCommand ( _collectionName, ele, cb, dmsCB,
                                 dpsCB, isSys ) ;
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "IndexDef:%s",
                           _index.toString().c_str() ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNDROPINDEX_DOIT, rc ) ;
      return rc ;
   }

   _rtnGet::_rtnGet ()
   : _options(),
     _hintExist( FALSE )
   {
   }

   _rtnGet::~_rtnGet ()
   {
   }

   const CHAR *_rtnGet::collectionFullName ()
   {
      return _options.getCLFullName() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_INIT, "_rtnGet::init" )
   INT32 _rtnGet::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                         const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                         const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNGET_INIT ) ;

      const CHAR * collection = NULL ;
      BSONObj hint( pHintBuff ) ;
      BSONObj realHint ;

      _options.setFlag( flags ) ;
      _options.setLimit( numToReturn ) ;
      _options.setSkip( numToSkip ) ;
      _options.setQuery( BSONObj( pMatcherBuff ) ) ;
      _options.setSelector( BSONObj( pSelectBuff ) ) ;
      _options.setOrderBy( BSONObj( pOrderByBuff ) ) ;

      rc = rtnGetStringElement( hint, FIELD_NAME_COLLECTION, &collection ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_COLLECTION, rc ) ;

      _options.setCLFullName( collection ) ;

      rc = rtnGetObjElement( hint, FIELD_NAME_HINT, realHint ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else
      {
         _options.setHint( realHint ) ;
         _hintExist = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_HINT, rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__RTNGET_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_DOIT, "_rtnGet::doit" )
   INT32 _rtnGet::doit( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                        SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                        INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNGET_DOIT ) ;

      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;

      rc = rtnGetCommandEntry ( type(), _options, cb, dmsCB, rtnCB,
                                *pContextID ) ;
      PD_TRACE_EXITRC ( SDB__RTNGET_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetCount)
   _rtnGetCount::_rtnGetCount ()
   {
   }

   _rtnGetCount::~_rtnGetCount ()
   {
   }

   const CHAR *_rtnGetCount::name ()
   {
      return NAME_GET_COUNT ;
   }

   RTN_COMMAND_TYPE _rtnGetCount::type ()
   {
      return CMD_GET_COUNT ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetIndexes)
   _rtnGetIndexes::_rtnGetIndexes ()
   {
   }

   _rtnGetIndexes::~_rtnGetIndexes ()
   {
   }

   const CHAR *_rtnGetIndexes::name ()
   {
      return NAME_GET_INDEXES ;
   }

   RTN_COMMAND_TYPE _rtnGetIndexes::type ()
   {
      return CMD_GET_INDEXES ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetDatablocks)
   _rtnGetDatablocks::_rtnGetDatablocks ()
   {
   }

   _rtnGetDatablocks::~_rtnGetDatablocks ()
   {
   }

   const CHAR *_rtnGetDatablocks::name ()
   {
      return NAME_GET_DATABLOCKS ;
   }

   RTN_COMMAND_TYPE _rtnGetDatablocks::type ()
   {
      return CMD_GET_DATABLOCKS ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetQueryMeta)
   _rtnGetQueryMeta::_rtnGetQueryMeta ()
   {
   }

   _rtnGetQueryMeta::~_rtnGetQueryMeta ()
   {
   }

   const CHAR *_rtnGetQueryMeta::name ()
   {
      return NAME_GET_QUERYMETA ;
   }

   RTN_COMMAND_TYPE _rtnGetQueryMeta::type ()
   {
      return CMD_GET_QUERYMETA ;
   }

   INT32 _rtnGetQueryMeta::doit( pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextDump *context = NULL ;

      SDB_ASSERT( pContextID, "context id can't be NULL" ) ;

      if ( !_hintExist )
      {
         /// compatiable with old version. Old version use selector for hint
         _options.setHint( _options.getSelector() ) ;
      }

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                              *pContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Create context failed, rc: %d", rc ) ;

      rc = context->open( BSONObj(), BSONObj(), -1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   *pContextID, rc ) ;

      // sample timetamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      rc = rtnGetQueryMeta( _options, dmsCB, cb, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] query meta failed, "
                   "matcher: %s, orderby: %s, hint: %s, rc: %d",
                   _options.getCLFullName(),
                   _options.getQuery().toString().c_str(),
                   _options.getOrderBy().toString().c_str(),
                   _options.getHint().toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnGetCollectionDetail)
   _rtnGetCollectionDetail::_rtnGetCollectionDetail ()
   {
   }

   _rtnGetCollectionDetail::~_rtnGetCollectionDetail ()
   {
   }

   const CHAR *_rtnGetCollectionDetail::name ()
   {
      return NAME_GET_CL_DETAIL ;
   }

   RTN_COMMAND_TYPE _rtnGetCollectionDetail::type ()
   {
      return CMD_GET_CL_DETAIL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRenameCollection)
   _rtnRenameCollection::_rtnRenameCollection ()
      :_clShortName ( NULL ), _newCLShortName ( NULL ), _csName( NULL )
   {
   }

   _rtnRenameCollection::~_rtnRenameCollection ()
   {
   }

   const CHAR *_rtnRenameCollection::name ()
   {
      return NAME_RENAME_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnRenameCollection::type ()
   {
      return CMD_RENAME_COLLECTION ;
   }

   BOOLEAN _rtnRenameCollection::writable ()
   {
      return TRUE ;
   }

   const CHAR *_rtnRenameCollection::collectionFullName ()
   {
      return _fullCollectionName.c_str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECL_INIT, "_rtnRenameCollection::init" )
   INT32 _rtnRenameCollection::init ( INT32 flags, INT64 numToSkip,
                                      INT64 numToReturn,
                                      const CHAR * pMatcherBuff,
                                      const CHAR * pSelectBuff,
                                      const CHAR * pOrderByBuff,
                                      const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECL_INIT ) ;

      try
      {
         BSONObj arg ( pMatcherBuff ) ;

         rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTIONSPACE, &_csName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[collectionspace]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_OLDNAME,
                                    &_clShortName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[oldname]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_NEWNAME,
                                    &_newCLShortName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[newname]" ) ;
            goto error ;
         }

         _fullCollectionName = _csName ;
         _fullCollectionName += "." ;
         _fullCollectionName += _clShortName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNRENAMECL_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECL_DOIT, "_rtnRenameCollection::doit" )
   INT32 _rtnRenameCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                      INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECL_DOIT ) ;

      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;

      // only allowed to rename cl by coord or standalone
      if ( SDB_ROLE_DATA == pmdGetDBRole() &&
           CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         rc = SDB_RTN_COORD_ONLY ;
         goto error ;
      }

      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextRenameCL *renameContext = NULL;
         rc = rtnCB->contextNew( RTN_CONTEXT_RENAMECL,
                                 (rtnContext **)&renameContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create context, rename cl failed, rc: %d",
                      rc );

         rc = renameContext->open( _csName, _clShortName, _newCLShortName,
                                   cb, w );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to open context, rename cl failed, rc: %d)",
                      rc );
      }
      else
      {
         rc = rtnRenameCollectionCommand( _csName, _clShortName,
                                          _newCLShortName, cb,
                                          dmsCB, dpsCB, TRUE ) ;
         string clFullName, newCLFullName ;
         clFullName = _csName ;
         clFullName += "." ;
         clFullName += _clShortName ;
         newCLFullName = _csName ;
         newCLFullName += "." ;
         newCLFullName += _newCLShortName ;
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           clFullName.c_str(), rc,
                           "NewName:%s", newCLFullName.c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNRENAMECL_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb );
         *pContextID = -1;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRenameCollectionSpace)
   _rtnRenameCollectionSpace::_rtnRenameCollectionSpace()
   {
      _oldName = NULL ;
      _newName = NULL ;
   }
   _rtnRenameCollectionSpace::~_rtnRenameCollectionSpace()
   {
   }

   INT32 _rtnRenameCollectionSpace::init ( INT32 flags, INT64 numToSkip,
                                           INT64 numToReturn,
                                           const CHAR * pMatcherBuff,
                                           const CHAR * pSelectBuff,
                                           const CHAR * pOrderByBuff,
                                           const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj arg ( pMatcherBuff ) ;
         rc = rtnGetStringElement ( arg, FIELD_NAME_OLDNAME,
                                    &_oldName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[oldname]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_NEWNAME,
                                    &_newName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[newname]" ) ;
            goto error ;
         }

         rc = dmsCheckCSName( _oldName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = dmsCheckCSName( _newName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRENAMECS_DOIT, "_rtnRenameCollectionSpace::doit" )
   INT32 _rtnRenameCollectionSpace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNRENAMECS_DOIT ) ;

      // only allowed to rename cs by coord or standalone
      if ( SDB_ROLE_DATA == pmdGetDBRole() &&
           CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         rc = SDB_RTN_COORD_ONLY ;
         goto error ;
      }

      SDB_ASSERT( cb, "cb can't be null!");
      SDB_ASSERT( rtnCB, "rtnCB can't be null!");
      SDB_ASSERT( pContextID, "pContextID can't be null!");
      *pContextID = -1;

      if ( CMD_SPACE_SERVICE_SHARD == getFromService() )
      {
         rtnContextRenameCS *renameContext = NULL ;
         rc = rtnCB->contextNew( RTN_CONTEXT_RENAMECS,
                                 (rtnContext **)&renameContext,
                                 *pContextID, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create context, rename cs failed, rc: %d",
                      rc ) ;

         rc = renameContext->open( _oldName, _newName, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to open context, rename cs failed, rc: %d)",
                      rc ) ;
      }
      else
      {
         rc = rtnRenameCollectionSpaceCommand( _oldName, _newName, cb,
                                               dmsCB, dpsCB, TRUE ) ;
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _oldName, rc, "NewName:%s", _newName ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNRENAMECS_DOIT, rc ) ;
      return rc ;
   error :
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done;
   }

   _rtnReorg::_rtnReorg ()
      :_collectionName ( NULL ), _hintBuffer ( NULL )
   {
   }

   _rtnReorg::~_rtnReorg ()
   {
   }

   INT32 _rtnReorg::spaceNode()
   {
      return CMD_SPACE_NODE_STANDALONE ;
   }

   const CHAR *_rtnReorg::collectionFullName ()
   {
      return _collectionName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREORG_INIT, "_rtnReorg::init" )
   INT32 _rtnReorg::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                           const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNREORG_INIT ) ;

      _hintBuffer = pHintBuff ;
      BSONObj arg ( pMatcherBuff ) ;
      rc = rtnGetStringElement ( arg, FIELD_NAME_COLLECTION,
                                 &_collectionName ) ;
      PD_TRACE_EXITRC ( SDB__RTNREORG_INIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREORG_DOIT, "_rtnReorg::doit" )
   INT32 _rtnReorg::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNREORG_DOIT ) ;
      BSONObj hint ( _hintBuffer ) ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto done ;
      }

      switch ( type() )
      {
         case CMD_REORG_OFFLINE :
            rc = rtnReorgOffline ( _collectionName, hint, cb, dmsCB, rtnCB ) ;
            break ;
         case CMD_REORG_ONLINE :
            PD_LOG ( PDERROR, "Online reorg is not supported yet" ) ;
            rc = SDB_SYS ;
            break ;
         case CMD_REORG_RECOVER :
            rc = rtnReorgRecover ( _collectionName, cb, dmsCB, rtnCB ) ;
            break ;
         default :
            PD_LOG ( PDEVENT, "Unknow reorg command" ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }
      dmsCB->writeDown ( cb ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "" ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNREORG_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgOffline)
   _rtnReorgOffline::_rtnReorgOffline ()
   {
   }

   _rtnReorgOffline::~_rtnReorgOffline ()
   {
   }

   const CHAR *_rtnReorgOffline::name ()
   {
      return NAME_REORG_OFFLINE ;
   }

   RTN_COMMAND_TYPE _rtnReorgOffline::type ()
   {
      return CMD_REORG_OFFLINE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgOnline)
   _rtnReorgOnline::_rtnReorgOnline ()
   {
   }

   _rtnReorgOnline::~_rtnReorgOnline ()
   {
   }

   const CHAR *_rtnReorgOnline::name ()
   {
      return NAME_REORG_ONLINE ;
   }

   RTN_COMMAND_TYPE _rtnReorgOnline::type ()
   {
      return CMD_REORG_ONLINE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReorgRecover)
   _rtnReorgRecover::_rtnReorgRecover ()
   {
   }

   _rtnReorgRecover::~_rtnReorgRecover ()
   {
   }

   const CHAR *_rtnReorgRecover::name ()
   {
      return NAME_REORG_RECOVER ;
   }

   RTN_COMMAND_TYPE _rtnReorgRecover::type ()
   {
      return CMD_REORG_RECOVER ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnShutdown)
   _rtnShutdown::_rtnShutdown ()
   {
   }

   _rtnShutdown::~_rtnShutdown ()
   {
   }

   const CHAR *_rtnShutdown::name ()
   {
      return NAME_SHUTDOWN ;
   }

   RTN_COMMAND_TYPE _rtnShutdown::type ()
   {
      return CMD_SHUTDOWN ;
   }

   INT32 _rtnShutdown::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR * pMatcherBuff,
                              const CHAR * pSelectBuff,
                              const CHAR * pOrderByBuff,
                              const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnShutdown::doit( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                             INT16 w , INT64 *pContextID )
   {
      PD_LOG( PDEVENT, "Shut down sevice" ) ;
      PMD_SHUTDOWN_DB( SDB_OK ) ;

      /// AUDIT
      PD_AUDIT_COMMAND( AUDIT_SYSTEM, name(), AUDIT_OBJ_NODE,
                        "", SDB_OK, "" ) ;

      return SDB_OK ;
   }

   _rtnTest::_rtnTest ()
      :_collectionName ( NULL )
   {
   }

   _rtnTest::~_rtnTest ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_INIT, "_rtnTest::init" )
   INT32 _rtnTest::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                          const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                          const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTEST_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      rc = rtnGetStringElement ( arg, FIELD_NAME_NAME, &_collectionName ) ;
      PD_TRACE_EXITRC ( SDB__RTNTEST_INIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_DOIT, "_rtnTest::doit" )
   INT32 _rtnTest::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                          INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTEST_DOIT ) ;

      switch ( type () )
      {
         case CMD_TEST_COLLECTION :
            rc = rtnTestCollectionCommand ( _collectionName , dmsCB ) ;
            break ;
         case CMD_TEST_COLLECTIONSPACE :
            rc = rtnTestCollectionSpaceCommand ( _collectionName, dmsCB ) ;
            break ;
         default :
            PD_LOG ( PDEVENT, "Unknow test command " ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      PD_TRACE_EXITRC ( SDB__RTNTEST_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTestCollection)
   _rtnTestCollection::_rtnTestCollection ()
   {
   }

   _rtnTestCollection::~_rtnTestCollection ()
   {
   }

   const CHAR *_rtnTestCollection::name ()
   {
      return NAME_TEST_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnTestCollection::type ()
   {
      return CMD_TEST_COLLECTION ;
   }

   const CHAR *_rtnTestCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTestCollectionspace)
   _rtnTestCollectionspace::_rtnTestCollectionspace ()
   {
   }

   _rtnTestCollectionspace::~_rtnTestCollectionspace ()
   {
   }

   const CHAR *_rtnTestCollectionspace::name ()
   {
      return NAME_TEST_COLLECTIONSPACE ;
   }

   RTN_COMMAND_TYPE _rtnTestCollectionspace::type ()
   {
      return CMD_TEST_COLLECTIONSPACE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSetPDLevel)
   _rtnSetPDLevel::_rtnSetPDLevel()
   {
      _pdLevel = PDEVENT ;
   }

   _rtnSetPDLevel::~_rtnSetPDLevel()
   {
   }

   const CHAR* _rtnSetPDLevel::name ()
   {
      return NAME_SET_PDLEVEL ;
   }

   RTN_COMMAND_TYPE _rtnSetPDLevel::type ()
   {
      return CMD_SET_PDLEVEL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_INIT, "_rtnSetPDLevel::init" )
   INT32 _rtnSetPDLevel::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff,
                                const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff,
                                const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNSETPDLEVEL_INIT ) ;
      BSONObj match ( pMatcherBuff ) ;

      rc = rtnGetIntElement( match, FIELD_NAME_PDLEVEL, _pdLevel ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to extract %s field for set pdleve",
                  FIELD_NAME_PDLEVEL ) ;
         goto error ;
      }
      if ( _pdLevel < PDSEVERE || _pdLevel > PDDEBUG )
      {
         PD_LOG ( PDWARNING, "PDLevel[%d] error, set to default[%d]",
                  _pdLevel, PDWARNING ) ;
         _pdLevel = PDWARNING ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNSETPDLEVEL_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_DOIT, "_rtnSetPDLevel::doit" )
   INT32 _rtnSetPDLevel::doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                                INT16 w, INT64 * pContextID )
   {
      PD_TRACE_ENTRY ( SDB__RTNSETPDLEVEL_DOIT ) ;
      setPDLevel( (PDLEVEL)_pdLevel ) ;
      PD_LOG ( getPDLevel(), "Set PDLEVEL to [%s]",
               getPDLevelDesp( getPDLevel() ) ) ;
      PD_TRACE_EXIT ( SDB__RTNSETPDLEVEL_DOIT ) ;
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReloadConfig)

   _rtnReloadConfig::_rtnReloadConfig()
   {
   }

   _rtnReloadConfig::~_rtnReloadConfig()
   {
   }

   const CHAR* _rtnReloadConfig::name()
   {
      return NAME_RELOAD_CONFIG ;
   }

   RTN_COMMAND_TYPE _rtnReloadConfig::type()
   {
      return CMD_RELOAD_CONFIG ;
   }

   INT32 _rtnReloadConfig::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR *pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnReloadConfig::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;

      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      pmdOptionsCB tmpCB ;
      BSONObj cfgObj ;

      rc = tmpCB.initFromFile( optCB->getConfFile(), FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init config file[%s] failed, rc: %d",
                 optCB->getConfFile(), rc ) ;
         goto error ;
      }

      rc = tmpCB.toBSON( cfgObj, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Convert config to bson failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = optCB->change( cfgObj, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Reload config[%s] failed, rc: %d",
                 cfgObj.toString().c_str(), rc ) ;
         goto error ;
      }
      /// dump memory config
      optCB->toBSON( cfgObj ) ;

      PD_LOG( PDEVENT, "Reload config succeed. All configs: %s",
              cfgObj.toString().c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _configOprBase::_configOprBase()
   {
   }

   _configOprBase::~_configOprBase()
   {
   }

   INT32 _configOprBase::_errorReport( BSONObj &returnObj )
   {
      INT32 rc = SDB_OK ;
      string returnStr;
      INT32 rebootCount = 0 ;
      BOOLEAN rebootFirstEntry  = TRUE ;
      INT32 forbidCount = 0 ;
      BOOLEAN forbidFirstEntry  = TRUE ;
      BSONElement rebootEle ;
      BSONElement forbidEle ;

      try
      {
         rebootEle = returnObj.getField( "Reboot" ) ;
         if ( Array == rebootEle.type() )
         {
            BSONObjIterator iter( rebootEle.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == rebootFirstEntry )
                  {
                     returnStr += "Config '" ;
                     returnStr +=  ele.valuestr() ;

                     rebootFirstEntry = FALSE ;
                  }
                  else
                  {
                     returnStr += ", '" ;
                     returnStr +=  ele.valuestr() ;
                  }
                  returnStr += "'" ;
                  rebootCount++ ;
               }
               if ( 3 == rebootCount )
               {
                  break ;
               }
            }
         }

         if ( rebootCount > 0 && rebootCount < 3 )
         {
            returnStr += " require(s) restart to take effect." ;
         }
         else if ( rebootCount == 3 )
         {
            returnStr += ", etc. require(s) restart to take effect." ;
         }

         forbidEle = returnObj.getField( "Forbidden" ) ;
         if ( Array == forbidEle.type() )
         {
            BSONObjIterator iter( forbidEle.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == forbidFirstEntry )
                  {
                     returnStr += " Config '" ;
                     returnStr +=  ele.valuestr() ;
                     forbidFirstEntry = FALSE ;
                  }
                  else
                  {
                     returnStr += ", '" ;
                     returnStr +=  ele.valuestr() ;
                  }
                  returnStr += "'" ;
                  forbidCount++ ;
               }
               if ( 3 == forbidCount )
               {
                  break ;
               }
            }
         }

         if ( forbidCount > 0 && forbidCount < 3 )
         {
            returnStr += " cannot be changed." ;
         }
         else if ( forbidCount == 3 )
         {
            returnStr += ", etc. cannot be changed." ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Exception during updateConf/deleteConf "
                 "info parsing: %s",
                 e.what() ) ;
         goto error ;
      }

      if ( rebootCount > 0 || forbidCount > 0 )
      {
         rc = SDB_RTN_CONF_NOT_TAKE_EFFECT ;
         PD_LOG_MSG( PDERROR, returnStr.c_str() ) ;
      }
done:
   return rc ;
error:
   goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnUpdateConfig)

   _rtnUpdateConfig::_rtnUpdateConfig()
   {
   }

   _rtnUpdateConfig::~_rtnUpdateConfig()
   {
   }

   const CHAR* _rtnUpdateConfig::name()
   {
      return NAME_UPDATE_CONFIG ;
   }

   RTN_COMMAND_TYPE _rtnUpdateConfig::type()
   {
      return CMD_UPDATE_CONFIG ;
   }

   INT32 _rtnUpdateConfig::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR *pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      BSONObj options = BSONObj( pMatcherBuff ) ;
      BSONObj cfgObj = options.getObjectField( FIELD_NAME_CONFIGS ) ;
      _newCfgObj.getOwned() ;
      BSONObjBuilder newObjBuilder ;

      try
      {
         BSONObjIterator iter( cfgObj );
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( ele.isNumber() || String == ele.type() )
            {
               newObjBuilder.append( ele ) ;
            }
            else if ( Bool == ele.type() )
            {
               newObjBuilder.append( ele.fieldName(), ele.Bool() ?
                                     "TRUE" : "FALSE" ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Field[%s] type[%d] is not "
                       "number/boolean/string", ele.fieldName(),
                       ele.type() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Exception during updateConf init: %s",
                 e.what() ) ;
      }

      _newCfgObj = newObjBuilder.obj() ;

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _rtnUpdateConfig::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      BSONObj returnObj ;
      pmdOptionsCB tmpOptionsCB ;
      BSONObj userConfig ;

      // check config value validity
      rc = tmpOptionsCB.restore( _newCfgObj, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Error while checking update configuration[%s], "
                 "rc: %d", _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      /*
      rc = tmpOptionsCB.toBSON( userConfig, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Convert config to bson failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = optCB->update( userConfig, FALSE, returnObj ) ;
      */
      rc = optCB->update( _newCfgObj, FALSE, returnObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update config[%s] failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = optCB->reflush2File( PMD_CFG_MASK_SKIP_UNFIELD |
                                PMD_CFG_MASK_MODE_LOCAL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Reflush update config[%s] to file failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = _errorReport( returnObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDeleteConfig)

   _rtnDeleteConfig::_rtnDeleteConfig()
   {
   }

   _rtnDeleteConfig::~_rtnDeleteConfig()
   {
   }

   const CHAR* _rtnDeleteConfig::name()
   {
      return NAME_DELETE_CONFIG ;
   }

   RTN_COMMAND_TYPE _rtnDeleteConfig::type()
   {
      return CMD_DELETE_CONFIG ;
   }

   INT32 _rtnDeleteConfig::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR *pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      BSONObj options = BSONObj( pMatcherBuff ) ;
      _newCfgObj = options.getObjectField( FIELD_NAME_CONFIGS ) ;
      _newCfgObj.getOwned() ;

      return SDB_OK ;
   }

   INT32 _rtnDeleteConfig::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      BSONObj returnObj ;
      BSONObj currentConf ;
      BSONObjBuilder deleteConfBuilder ;

      rc = optCB->toBSON( currentConf, 0 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Construct delete config[%s] failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      try
      {
         BSONObj::iterator confIter = currentConf.begin() ;
         while ( confIter.more() )
         {
            BSONElement elem = confIter.next() ;
            if ( !_newCfgObj.hasField( elem.fieldName() ) )
            {
               deleteConfBuilder.append( elem ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Exception during deleteConf contructing: %s",
                 e.what() ) ;
         goto error ;
      }

      rc = optCB->update( deleteConfBuilder.obj(), TRUE, returnObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Delete config[%s] failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = optCB->reflush2File( PMD_CFG_MASK_SKIP_UNFIELD |
                                PMD_CFG_MASK_MODE_LOCAL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Reflush update config[%s] to file failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = _errorReport( returnObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStart)
   _rtnTraceStart::_rtnTraceStart ()
   {
      _mask = 0xFFFFFFFF ;
      _size = TRACE_DFT_BUFFER_SIZE ;
   }
   _rtnTraceStart::~_rtnTraceStart ()
   {
   }

   const CHAR *_rtnTraceStart::name ()
   {
      return CMD_NAME_TRACE_START ;
   }

   RTN_COMMAND_TYPE _rtnTraceStart::type ()
   {
      return CMD_TRACE_START ;
   }

   BOOLEAN _rtnTraceStart::_isFunctionNameValid( const CHAR* verifiedFuncName )
   {
      BOOLEAN isValid = FALSE ;
      for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
      {
         const CHAR* fullFuncName = pdGetTraceFunction( i ) ;
         const CHAR* pTemp = ossStrchr( fullFuncName, ':' ) ;

         // eg: fullFuncName = "_authCB::authenticate"
         // first case: verifiedFuncName = "authenticate"
         if( pTemp != NULL )
         {
            // pTemp = "::authenticate"
            ++pTemp ;
            ++pTemp ;
            if( 0 == ossStrcmp( pTemp, verifiedFuncName ) )
            {
               isValid = TRUE ;
               _functionNameId.push_back ( i ) ;
               // do NOT break althought however we may have functions
               // with duplicate names
               continue ;
            }
         }

         // second case: verifiedFuncName = "_authCB::authenticate"
         if( 0 == ossStrcmp( fullFuncName, verifiedFuncName ) )
         {
            isValid = TRUE ;
            _functionNameId.push_back ( i ) ;
            // do NOT break althought however we may have functions
            // with duplicate names
         }
      }

      return isValid ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_INIT, "_rtnTraceStart::init" )
   INT32 _rtnTraceStart::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_INIT ) ;
      try
      {
         UINT32 one = 1 ;
         BSONObj arg ( pMatcherBuff );
         BSONElement eleSize = arg.getField ( FIELD_NAME_SIZE ) ;
         BSONElement eleComp = arg.getField( FIELD_NAME_COMPONENTS );
         BSONElement eleBreakPoint = arg.getField( FIELD_NAME_BREAKPOINTS );
         BSONElement eleTID = arg.getField ( FIELD_NAME_THREADS ) ;
         BSONElement eleFunctionNames = arg.getField ( FIELD_NAME_FUNCTIONNAMES ) ;
         BSONElement eleThreadTypes = arg.getField ( FIELD_NAME_THREADTYPES ) ;

         // init buffsize
         if ( eleSize.isNumber() )
         {
            _size = (UINT32)eleSize.numberLong() ;
         }

         // init components
         if ( eleComp.type() == Array )
         {
            _mask = 0 ;
            BSONObjIterator it ( eleComp.embeddedObject() ) ;
            if ( !it.more () )
            {
               // if there's no element, that means we need mask everything
               for( UINT32 i = 0; i < pdGetTraceComponentSize() ; ++i )
               {
                  _mask |= ( one << i ) ;
               }
            }
            else
            {
               while ( it.more() )
               {
                  BSONElement ele = it.next() ;
                  if ( ele.type() == String )
                  {
                     for ( UINT32 i = 0; i < pdGetTraceComponentSize() ; ++i )
                     {
                        if ( 0 == ossStrcmp ( pdGetTraceComponent(i),
                                              ele.valuestr() ) )
                        {
                           _mask |= ( one << i ) ;
                        }
                     } // for
                  } // if ( ele.type() == String )
               } // while ( it.more() )
            }
         } // if ( eleComp.type() == Array )

#if defined (SDB_ENGINE)
         // init breakPoints
         if( eleBreakPoint.type() == Array )
         {
            BSONObjIterator it( eleBreakPoint.embeddedObject() );
            while( it.more() )
            {
               BSONElement ele = it.next();
               if( ele.type() == String )
               {
                  const char * eleStr = ele.valuestr();
                  for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
                  {
                     const CHAR *  funcName = pdGetTraceFunction( i );
                     if( 0 == ossStrcmp( funcName, eleStr ) )
                     {
                        _breakPoint.push_back ( i ) ;
                        // do NOT break since we may have functions with
                        // duplicate names
                     } // if( 0 == ossStrcmp( funcName, eleStr ) )
                  } // for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
               } // if( ele.type() == String )
            } // while( it.more() )
         }
#endif

         // init tids
         if ( eleTID.type() == Array )
         {
            BSONObjIterator it ( eleTID.embeddedObject() ) ;
            while ( it.more () )
            {
               BSONElement ele = it.next() ;
               if ( ele.isNumber() )
               {
                  UINT32 tid = (UINT32)ele.numberInt() ;
                  _tid.push_back ( tid ) ;
               }
            }
         }

         // init functionNames
         if ( eleFunctionNames.type() == Array )
         {
            BSONObjIterator it ( eleFunctionNames.embeddedObject() ) ;
            while ( it.more () )
            {
               BSONElement ele = it.next() ;
               if ( ele.type() == String )
               {
                  const CHAR* eleStr = ele.valuestr();
                  BOOLEAN isValid = FALSE ;
                  isValid = _isFunctionNameValid( eleStr ) ;
                  if( !isValid )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR,
                                 "The function name: %s is invalid", eleStr ) ;
                     goto error ;
                  }
               }
            }
         }

         // init threadTypes
         if ( eleThreadTypes.type() == Array )
         {
            pmdEPFactory &factory = pmdGetEPFactory() ;
            BSONObjIterator it ( eleThreadTypes.embeddedObject() ) ;
            while ( it.more () )
            {
               BSONElement ele = it.next() ;
               if ( ele.type() == String )
               {
                  const CHAR* eleStr = ele.valuestr() ;
                  INT32 threadType = factory.name2Type( eleStr ) ;
                  if( threadType >= 0 )
                  {
                     _threadType.push_back( threadType ) ;
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR,
                                 "The thread type: %s is invalid", eleStr ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when initialize trace start command: %s",
                       e.what() ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTART_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_DOIT, "_rtnTraceStart::doit" )
   INT32 _rtnTraceStart::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_DOIT ) ;
      rc = sdbGetPDTraceCB()->start ( (UINT64)_size, _mask, &_breakPoint, &_tid,
                                      &_functionNameId, &_threadType ) ;
      PD_TRACE_EXITRC ( SDB__RTNTRACESTART_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceResume)
   _rtnTraceResume::_rtnTraceResume ()
   {
   }
   _rtnTraceResume::~_rtnTraceResume ()
   {
   }

   const CHAR *_rtnTraceResume::name ()
   {
      return CMD_NAME_TRACE_RESUME ;
   }

   RTN_COMMAND_TYPE _rtnTraceResume::type ()
   {
      return CMD_TRACE_RESUME ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_INIT, "_rtnTraceResume::init" )
   INT32 _rtnTraceResume::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_INIT ) ;
      PD_TRACE_EXITRC ( SDB__RTNTRACERESUME_INIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_DOIT, "_rtnTraceResume::doit" )
   INT32 _rtnTraceResume::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_DOIT ) ;
      pdTraceCB *pdTraceCB = sdbGetPDTraceCB() ;
      pdTraceCB->removeAllBreakPoint () ;
      // sleep for a second so that break point removal information is broadcast
      // to all CPUs
      // Note this is not performance sensitive code, so it's safe to sleep for
      // 1 second
      ossSleepsecs(1) ;
      pdTraceCB->resumePausedEDUs () ;
      PD_TRACE_EXITRC ( SDB__RTNTRACERESUME_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStop)
   _rtnTraceStop::_rtnTraceStop ()
   {
      _pDumpFileName = FALSE ;
   }
   _rtnTraceStop::~_rtnTraceStop ()
   {
   }

   const CHAR *_rtnTraceStop::name ()
   {
      return CMD_NAME_TRACE_STOP ;
   }

   RTN_COMMAND_TYPE _rtnTraceStop::type ()
   {
      return CMD_TRACE_STOP ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_INIT, "_rtnTraceStop::init" )
   INT32 _rtnTraceStop::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                               const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                               const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTOP_INIT ) ;
      try
      {
         BSONObj arg ( pMatcherBuff ) ;
         BSONElement eleDumpName = arg.getField ( FIELD_NAME_FILENAME ) ;
         if ( eleDumpName.type() == String )
         {
            _pDumpFileName = eleDumpName.valuestr() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when initialize trace stop command: %s",
                       e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTOP_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_DOIT, "_rtnTraceStop::doit" )
   INT32 _rtnTraceStop::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                               _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                               INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTOP_DOIT ) ;
      pdTraceCB *traceCB = sdbGetPDTraceCB() ;
      OSSFILE outFile ;
      BOOLEAN isOpen = FALSE ;
      CHAR filePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR path[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR *ptr = NULL ;

      if ( _pDumpFileName && ossStrlen( _pDumpFileName ) > 0 &&
           traceCB->getSize() > 0 )
      {
         traceCB->stop() ;

#ifdef _LINUX
         if ( OSS_FILE_SEP_CHAR != _pDumpFileName[ 0 ] )
#else
         if ( NULL == ossStrchr( _pDumpFileName, ':' ) )
#endif // _LINUX
         {
            pmdOptionsCB *pOptCB = pmdGetKRCB()->getOptionCB() ;
            utilBuildFullPath( pOptCB->getDiagLogPath(),
                               _pDumpFileName,
                               OSS_MAX_PATHSIZE,
                               filePath ) ;
         }
         else
         {
            ossStrncpy( filePath, _pDumpFileName, OSS_MAX_PATHSIZE ) ;
         }

         //create dir
         ossStrncpy( path, filePath, OSS_MAX_PATHSIZE ) ;
         ptr = ossStrrchr( path, OSS_FILE_SEP_CHAR ) ;
         if ( ptr != &path[0] )
         {
            *ptr = 0 ;
            if ( ossAccess( path ) )
            {
               ossMkdir( path ) ;
            }
         }

         if( SDB_OK == ossAccess( filePath ) )
         {
            if( !traceCB->isTraceFile( filePath ) )
            {
               rc = SDB_FE ;
               PD_LOG( PDERROR, "%s exists. But it isn't trace file",
                       filePath ) ;
               goto error ;
            }
         }

         /// open file
         rc = ossOpen( filePath, OSS_REPLACE|OSS_READWRITE,
                       OSS_DEFAULTFILE, outFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open file[%s] failed, rc: %d",
                    filePath, rc ) ;
            goto error ;
         }
         isOpen = TRUE ;

         PD_LOG( PDEVENT, "Begin to dump trace to %s ...", filePath ) ;
         rc = traceCB->dump( &outFile ) ;
         PD_RC_CHECK ( rc, PDWARNING, "Failed to stop trace, rc = %d", rc ) ;

         ossClose( outFile ) ;
      }
   done :
      traceCB->destroy() ;
      PD_TRACE_EXITRC ( SDB__RTNTRACESTOP_DOIT, rc ) ;
      return rc ;
   error :
      if ( isOpen )
      {
         ossClose( outFile ) ;
         ossDelete( filePath ) ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTraceStatus)
   _rtnTraceStatus::_rtnTraceStatus ()
   {
   }
   _rtnTraceStatus::~_rtnTraceStatus ()
   {
   }

   const CHAR *_rtnTraceStatus::name ()
   {
      return CMD_NAME_TRACE_STATUS ;
   }

   RTN_COMMAND_TYPE _rtnTraceStatus::type ()
   {
      return CMD_TRACE_STATUS ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_INIT, "_rtnTraceStatus::init" )
   INT32 _rtnTraceStatus::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                 const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTATUS_INIT ) ;
      _flags = flags ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip ;
      _matcherBuff = pMatcherBuff ;
      _selectBuff = pSelectBuff ;
      _orderByBuff = pOrderByBuff ;

      PD_TRACE_EXITRC ( SDB__RTNTRACESTATUS_INIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_DOIT, "_rtnTraceStatus::doit" )
   INT32 _rtnTraceStatus::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTATUS_DOIT ) ;
      rtnContextDump *context = NULL ;
      *pContextID = -1 ;
      // create cursors
      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                               *pContextID, cb ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to create new context, rc = %d", rc ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      try
      {
         BSONObj matcher ( _matcherBuff ) ;
         BSONObj selector ( _selectBuff ) ;
         BSONObj orderBy ( _orderByBuff ) ;

         rc = context->open( selector,
                             matcher,
                             orderBy.isEmpty() ? _numToReturn : -1,
                             orderBy.isEmpty() ? _numToSkip : 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

         rc = monDumpTraceStatus ( context ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to dump trace status, rc = %d", rc ) ;

         if ( !orderBy.isEmpty() )
         {
            rc = rtnSort( (rtnContext**)&context,
                          orderBy,
                          cb, _numToSkip,
                          _numToReturn, *pContextID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when extracting trace status: %s", e.what() );
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNTRACESTATUS_DOIT, rc ) ;
      return rc ;
   error :
      if ( context )
      {
         rtnCB->contextDelete ( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnLoad) ;
   _rtnLoad::_rtnLoad ()
   {
      _parameters.pFieldArray = NULL ;
   }
   _rtnLoad::~_rtnLoad ()
   {
      SAFE_OSS_FREE ( _parameters.pFieldArray ) ;
   }

   const CHAR *_rtnLoad::name ()
   {
      return CMD_NAME_JSON_LOAD ;
   }

   RTN_COMMAND_TYPE _rtnLoad::type ()
   {
      return CMD_JSON_LOAD ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOAD_INIT, "_rtnLoad::init" )
   INT32 _rtnLoad::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                          const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                          const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNLOAD_INIT ) ;
      MIG_PARSER_FILE_TYPE fileType       = MIG_PARSER_JSON ;
      UINT32               threadNum      = MIG_THREAD_NUMBER ;
      UINT32               bucketNum      = MIG_BUCKET_NUMBER ;
      UINT32               bufferSize     = MIG_SUM_BUFFER_SIZE ;
      BOOLEAN              isAsynchronous = FALSE ;
      BOOLEAN              headerline     = FALSE ;
      CHAR                *fields         = NULL ;
      const CHAR          *tempValue      = NULL ;
      CHAR                 delCFR[4] ;
      BSONElement          tempEle ;

      ossMemset ( _fileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset ( _csName,   0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset ( _clName,   0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      delCFR[0] = '"' ;
      delCFR[1] = ',' ;
      delCFR[2] = '\n' ;
      delCFR[3] = 0 ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         //file name
         tempEle = obj.getField ( FIELD_NAME_FILENAME ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No file name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "file name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _fileName, tempValue, tempEle.valuestrsize() ) ;

         //cs name
         tempEle = obj.getField ( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No collection space name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "collection space name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _csName, tempValue, tempEle.valuestrsize() ) ;

         //cl name
         tempEle = obj.getField ( FIELD_NAME_COLLECTION ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "No collection name" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( String != tempEle.type() )
         {
            PD_LOG ( PDERROR, "collection name is not a string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tempValue = tempEle.valuestr() ;
         ossStrncpy ( _clName, tempValue, tempEle.valuestrsize() ) ;

         //fields
         tempEle = obj.getField ( FIELD_NAME_FIELDS ) ;
         if ( !tempEle.eoo() )
         {
            INT32 fieldsSize = 0 ;
            if ( String != tempEle.type() )
            {
               PD_LOG ( PDERROR, "fields is not a string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            tempValue = tempEle.valuestr() ;
            fieldsSize = tempEle.valuestrsize() ;
            fields = (CHAR *)SDB_OSS_MALLOC( fieldsSize ) ;
            if ( !fields )
            {
               PD_LOG ( PDERROR,
                        "Unable to allocate %d bytes memory", fieldsSize ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            ossStrncpy ( fields,
                         tempValue, fieldsSize ) ;
         }

         //character
         tempEle = obj.getField ( FIELD_NAME_CHARACTER ) ;
         if ( !tempEle.eoo() )
         {
            INT32 fieldsSize = 0 ;
            if ( String != tempEle.type() )
            {
               PD_LOG ( PDERROR, "fields is not a string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            tempValue = tempEle.valuestr() ;
            fieldsSize = tempEle.valuestrsize() ;
            if ( fieldsSize == 4 )
            {
               ossStrncpy ( delCFR, tempValue, fieldsSize ) ;
               delCFR[3] = 0 ;
            }
            else
            {
               PD_LOG ( PDERROR, "error character" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // asynchronous
         tempEle = obj.getField ( FIELD_NAME_ASYNCHRONOUS ) ;
         if ( !tempEle.eoo() )
         {
            if ( Bool != tempEle.type() )
            {
               PD_LOG ( PDERROR, "asynchronous is not a bool" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            isAsynchronous = tempEle.boolean() ;
         }

         // headerline
         tempEle = obj.getField ( FIELD_NAME_HEADERLINE ) ;
         if ( !tempEle.eoo() )
         {
            if ( Bool != tempEle.type() )
            {
               PD_LOG ( PDERROR, "headerline is not a bool" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            headerline = tempEle.boolean() ;
         }

         // thread number
         tempEle = obj.getField ( FIELD_NAME_THREADNUM ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "thread number is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            threadNum = (UINT32)tempEle.Int() ;
         }

         // bucket number
         tempEle = obj.getField ( FIELD_NAME_BUCKETNUM ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "bucket number is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            bucketNum = (UINT32)tempEle.Int() ;
         }

         // buffer size
         tempEle = obj.getField ( FIELD_NAME_PARSEBUFFERSIZE ) ;
         if ( !tempEle.eoo() )
         {
            if ( NumberInt != tempEle.type() )
            {
               PD_LOG ( PDERROR, "buffer size is not a number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            bufferSize = (UINT32)tempEle.Int() ;
         }

         // type
         tempEle = obj.getField ( FIELD_NAME_LTYPE ) ;
         if ( tempEle.eoo() )
         {
            PD_LOG ( PDERROR, "file type must input" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( NumberInt != tempEle.type() )
         {
            PD_LOG ( PDERROR, "file type is not a number" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fileType = (MIG_PARSER_FILE_TYPE)tempEle.Int() ;

      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _parameters.pFieldArray          = fields ;
      _parameters.headerline           = headerline ;
      _parameters.fileType             = fileType ;
      _parameters.pFileName            = _fileName ;
      _parameters.pCollectionSpaceName = _csName ;
      _parameters.pCollectionName      = _clName ;
      _parameters.bufferSize           = bufferSize ;
      _parameters.bucketNum            = bucketNum ;
      _parameters.workerNum            = threadNum ;
      _parameters.isAsynchronous       = isAsynchronous ;
      _parameters.port                 = NULL ;
      _parameters.delCFR[0]            = delCFR[0] ;
      _parameters.delCFR[1]            = delCFR[1] ;
      _parameters.delCFR[2]            = delCFR[2] ;
      _parameters.delCFR[3]            = delCFR[3] ;
   done:
      PD_TRACE_EXITRC ( SDB__RTNLOAD_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOAD_DOIT, "_rtnLoad::doit" )
   INT32 _rtnLoad::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                          INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__RTNLOAD_DOIT ) ;
      migMaster master ;
      _parameters.clientSock = cb->getClientSock() ;

      rc = master.initialize( &_parameters ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init migMaster, rc=%d", rc ) ;
         goto error ;
      }

      rc = master.run( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run migMaster, rc=%d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNLOAD_DOIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnExportConf)

   _rtnExportConf::_rtnExportConf()
   {
      _mask = 0 ;
   }

   INT32 _rtnExportConf::init( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR *pMatcherBuff,
                               const CHAR *pSelectBuff,
                               const CHAR *pOrderByBuff,
                               const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_TYPE ) ;
         if ( e.eoo() )
         {
            _mask = PMD_CFG_MASK_SKIP_UNFIELD ;
         }
         else if ( e.isNumber() )
         {
            INT32 type = e.numberInt() ;
            /// 0: ignore none
            /// 1: ignore hide default
            /// 2: ignore default
            /// 3: ignore unfield
            switch( type )
            {
               case 0 :
                  _mask = 0 ;
                  break ;
               case 1 :
                  _mask = PMD_CFG_MASK_SKIP_HIDEDFT ;
                  break ;
               case 2 :
                  _mask = PMD_CFG_MASK_SKIP_HIDEDFT | PMD_CFG_MASK_SKIP_NORMALDFT ;
                  break ;
               default :
                  _mask = PMD_CFG_MASK_SKIP_UNFIELD ;
                  break ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Field[%s] should be numberInt",
                    e.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNEXPCONF, "_rtnExportConf::doit" )
   INT32 _rtnExportConf::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNEXPCONF ) ;
      rc = pmdGetKRCB()->getOptionCB()->reflush2File( _mask ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to export configration:%d",rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNEXPCONF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRemoveBackup)
   _rtnRemoveBackup::_rtnRemoveBackup()
   {
      _path          = NULL ;
      _backupName    = NULL ;
      _matcherBuff   = NULL ;
   }

   INT32 _rtnRemoveBackup::init( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      _matcherBuff = pMatcherBuff ;
      try
      {
         BSONObj matcher( _matcherBuff ) ;
         rc = rtnGetStringElement( matcher, FIELD_NAME_PATH, &_path ) ;
         if ( rc == SDB_FIELD_NOT_EXIST )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_PATH, rc ) ;
         rc = rtnGetStringElement( matcher, FIELD_NAME_NAME, &_backupName ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnRemoveBackup::doit( pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj matcher( _matcherBuff ) ;
         rc = rtnRemoveBackup( cb, _path, _backupName, matcher ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
      }
      return rc ;
   }

   _rtnForceSession::_rtnForceSession()
   :_sessionID( OSS_INVALID_PID )
   {

   }

   _rtnForceSession::~_rtnForceSession()
   {

   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnForceSession )
   INT32 _rtnForceSession::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                  const CHAR *pMatcherBuff,
                                  const CHAR *pSelectBuff,
                                  const CHAR *pOrderByBuff,
                                  const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj obj( pMatcherBuff ) ;
         BSONElement sessionID = obj.getField( FIELD_NAME_SESSIONID ) ;
         if ( !sessionID.isNumber() )
         {
            PD_LOG( PDERROR, "session id should be a number:%s",
                    obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _sessionID = sessionID.numberLong() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnForceSession::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                  _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                  INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *mgr = pmdGetKRCB()->getEDUMgr() ;

      if ( _sessionID == cb->getID() )
      {
         PD_LOG( PDDEBUG, "Force self[%lld], ignored", _sessionID ) ;
         goto done ;
      }

      rc = mgr->forceUserEDU( _sessionID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to force edu:%lld, rc:%d",
                 _sessionID, rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDEVENT, "Force session[%lld] succeed", _sessionID ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListLob)
   _rtnListLob::_rtnListLob()
   :_contextID( -1 ), _skip( 0 ), _returnNum( -1 ),
    _fullName( NULL )
   {
   }

   _rtnListLob::~_rtnListLob()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLISTLOB_INIT, "_rtnListLob::init" )
   INT32 _rtnListLob::init( INT32 flags, INT64 numToSkip,
                            INT64 numToReturn,
                            const CHAR *pMatcherBuff,
                            const CHAR *pSelectBuff,
                            const CHAR *pOrderByBuff,
                            const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLISTLOB_INIT ) ;
      try
      {
         _query = BSONObj( pMatcherBuff ) ;
         _selector = BSONObj( pSelectBuff ) ;
         _orderBy = BSONObj( pOrderByBuff ) ;
         _hint = BSONObj( pHintBuff ) ;
         _skip = numToSkip ;
         _returnNum = numToReturn ;

         BSONElement ele = _hint.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            // old version message, need to move query to hint
            _hint = _query ;
            _query = BSONObj() ;

            ele = _hint.getField( FIELD_NAME_COLLECTION ) ;
         }

         _fullName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLISTLOB_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_rtnListLob::collectionFullName()
   {
      return _fullName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLISTLOB_DOIT, "_rtnListLob::doit" )
   INT32 _rtnListLob::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                             INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLISTLOB_DOIT ) ;
      rtnContextListLob *context = NULL ;

      rc = rtnCB->contextNew( RTN_CONTEXT_LIST_LOB,
                              (rtnContext**)(&context),
                              _contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob context:%d", rc ) ;
         goto error ;
      }

      if ( _orderBy.isEmpty() )
      {
         rc = context->open( _query, _selector, _hint, _skip, _returnNum, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open list lob context:%d", rc ) ;
      }
      else
      {
         rc = context->open( _query, _selector, _hint, 0, -1, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open list lob context:%d", rc ) ;

         rc = rtnSort( (rtnContext**)&context, _orderBy, cb,
                       _skip, _returnNum, _contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

      *pContextID = _contextID ;
   done:
      PD_TRACE_EXITRC( SDB__RTNLISTLOB_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != _contextID )
      {
         rtnCB->contextDelete ( _contextID, cb ) ;
         _contextID = -1 ;
      }
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSetSessionAttr )

   /*
      _rtnLocalSessionProp implement
   */
   _rtnLocalSessionProp::_rtnLocalSessionProp()
   {
      _cb = NULL ;
   }

   void _rtnLocalSessionProp::bindEDU( _pmdEDUCB * cb )
   {
      _cb = cb ;
   }

   void _rtnLocalSessionProp::_toBson( BSONObjBuilder &builder ) const
   {
      if ( _cb )
      {
         _cb->getTransExecutor()->toBson( builder ) ;

         if ( _cb->getSource() )
         {
            try
            {
               builder.append( FIELD_NAME_SOURCE, _cb->getSource() ) ;
            }
            catch( std::exception &e )
            {
               /// ignore
               PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
            }
         }
      }
   }

   INT32 _rtnLocalSessionProp::_checkTransConf( const _dpsTransConfItem *pTransConf )
   {
      INT32 rc = SDB_OK ;

      UINT32 mask = pTransConf->getTransConfMask() ;
      /// When in transaction, can only update transtimeout
      if ( _cb && _cb->isTransaction() )
      {
         OSS_BIT_CLEAR( mask, TRANS_CONF_MASK_TIMEOUT ) ;

         if ( 0 != mask )
         {
            PD_LOG_MSG( PDERROR, "In transaction can only update %s",
                        FIELD_NAME_TRANS_TIMEOUT ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( !sdbGetTransCB()->isTransOn() &&
                OSS_BIT_TEST( mask, TRANS_CONF_MASK_AUTOCOMMIT ) &&
                pTransConf->isTransAutoCommit() )
      {
         rc = SDB_DPS_TRANS_DIABLED ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLocalSessionProp::_updateTransConf( const _dpsTransConfItem *pTransConf )
   {
      if ( _cb )
      {
         _cb->getTransExecutor()->updateByMask( *pTransConf ) ;
      }
   }

   void _rtnLocalSessionProp::_updateSource( const CHAR *pSource )
   {
      if ( _cb )
      {
         _cb->setSource( pSource ) ;
      }
   }

   INT32 _rtnSetSessionAttr::init( INT32 flags, INT64 numToSkip,
                                   INT64 numToReturn,
                                   const CHAR *pMatcherBuff,
                                   const CHAR *pSelectBuff,
                                   const CHAR *pOrderByBuff,
                                   const CHAR *pHintBuff )
   {
      _pMatchBuff = pMatcherBuff ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSETSESSATTR_DOIT, "_rtnSetSessionAttr::doit" )
   INT32 _rtnSetSessionAttr::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                   _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                   INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSETSESSATTR_DOIT ) ;

      try
      {
         BSONObj property( _pMatchBuff ) ;
         _rtnLocalSessionProp sessionProp ;
         sessionProp.bindEDU( cb ) ;

         rc = sessionProp.parseProperty( property ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse session property, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to set sessionAttr, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSETSESSATTR_DOIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnGetSessionAttr )

   _rtnGetSessionAttr::_rtnGetSessionAttr ()
   {
   }

   _rtnGetSessionAttr::~_rtnGetSessionAttr ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNGETSESSATTR_INIT, "_rtnGetSessionAttr::init" )
   INT32 _rtnGetSessionAttr::init ( INT32 flags, INT64 numToSkip,
                                    INT64 numToReturn,
                                    const CHAR * pMatcherBuff,
                                    const CHAR * pSelectBuff,
                                    const CHAR * pOrderByBuff,
                                    const CHAR * pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnGetSessionAttr::doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                    _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                                    INT16 w, INT64 * pContextID )
   {
      _rtnLocalSessionProp sessionProp ;
      sessionProp.bindEDU( cb ) ;

      _buff = rtnContextBuf( sessionProp.toBSON() ) ;

      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnTruncate)
   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNTRUNCATE_INIT, "_rtnTruncate::init" )
   INT32 _rtnTruncate::init( INT32 flags, INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pMatcherBuff,
                             const CHAR *pSelectBuff,
                             const CHAR *pOrderByBuff,
                             const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNTRUNCATE_INIT ) ;
      try
      {
         BSONObj query( pMatcherBuff ) ;
         BSONElement ele = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid collection name:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _fullName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNTRUNCATE_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNTRUNCATE_DOIT, "_rtnTruncate::doit" )
   INT32 _rtnTruncate::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                             INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNTRUNCATE_DOIT ) ;
      rc = rtnTruncCollectionCommand( _fullName,
                                      cb,
                                      dmsCB,
                                      dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate collection[%s], rc:%d",
                 _fullName, rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNTRUNCATE_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnPop )
   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNPOP_INIT, "_rtnPop::init" )
   INT32 _rtnPop::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                         const CHAR *pMatcherBuff,
                         const CHAR *pSelectBuff,
                         const CHAR *pOrderByBuff,
                         const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNPOP_INIT ) ;
      try
      {
         BSONObj query( pMatcherBuff ) ;
         BSONElement eleName, eleLID, eleDirection ;
         eleName = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != eleName.type() )
         {
            PD_LOG( PDERROR, "Invalid collection name in command: %s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _fullName = eleName.valuestr() ;

         eleLID = query.getField( FIELD_NAME_LOGICAL_ID ) ;
         if ( EOO == eleLID.type() )
         {
            PD_LOG( PDERROR, "Logical id for pop operation is not provided: %s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( NumberLong != eleLID.type()
              && NumberInt != eleLID.type()
              && NumberDouble != eleLID.type() )
         {
            PD_LOG( PDERROR, "Type for LogicalID is invalid:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _logicalID = eleLID.numberLong() ;

         eleDirection = query.getField( FIELD_NAME_DIRECTION ) ;
         if ( EOO == eleDirection.type() )
         {
            _direction = 1 ;
         }
         else
         {
            if ( NumberInt != eleDirection.type() )
            {
               PD_LOG( PDERROR, "Type for Direction is invalid: %s",
                       query.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               _direction = eleDirection.numberInt() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNPOP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNPOP_DOIT, "_rtnPop::doit" )
   INT32 _rtnPop::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                         _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                         INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNPOP_DOIT ) ;

      PD_LOG( PDDEBUG, "Pop collection[%s]: LogicalID[%lld], Direction[%d]",
              _fullName, _logicalID, (INT32)_direction ) ;

      rc = rtnPopCommand( _fullName, _logicalID, cb,
                          dmsCB, dpsCB, _direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pop record from collection[%s], "
                   "rc: %d", _fullName, rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNPOP_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSyncDB )
   _rtnSyncDB::_rtnSyncDB()
   {
      _syncType = 1 ;
      _csName = NULL ;
      _block = FALSE ;
   }

   _rtnSyncDB::~_rtnSyncDB()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSYNCDB_INIT, "_rtnSyncDB::init" )
   INT32 _rtnSyncDB::init ( INT32 flags, INT64 numToSkip,
                            INT64 numToReturn,
                            const CHAR *pMatcherBuff,
                            const CHAR *pSelectBuff,
                            const CHAR *pOrderByBuff,
                            const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_DEEP ) ;
         if ( e.isNumber() )
         {
            _syncType = (INT32)e.numberInt() ;
         }
         else if ( e.isBoolean() )
         {
            _syncType = e.boolean() ? 1 : 0 ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_DEEP, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = matcher.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            _csName = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = matcher.getField( FIELD_NAME_BLOCK ) ;
         if ( Bool == e.type() )
         {
            _block = e.Bool() ? TRUE : FALSE ;
         }
         else if ( e.isNumber() )
         {
            _block = ( 0 == e.numberInt() ) ? FALSE : TRUE ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_BLOCK, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSYNCDB_DOIT, "_rtnSyncDB::doit" )
   INT32 _rtnSyncDB::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                            _SDB_RTNCB *rtnCB, _dpsLogWrapper *d,
                            INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSYNCDB_DOIT ) ;

      rc = rtnSyncDB( cb, _syncType, _csName, _block ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to sync db: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSYNCDB_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnLoadCollectionSpace )
   _rtnLoadCollectionSpace::_rtnLoadCollectionSpace()
   : _csName( NULL ),
     _needChangeID( FALSE ),
     _csUniqueID( UTIL_CSUNIQUEID_LOADCS )
   {
   }
   _rtnLoadCollectionSpace::~_rtnLoadCollectionSpace()
   {
   }

   INT32 _rtnLoadCollectionSpace::init( INT32 flags, INT64 numToSkip,
                                        INT64 numToReturn,
                                        const CHAR * pMatcherBuff,
                                        const CHAR * pSelectBuff,
                                        const CHAR * pOrderByBuff,
                                        const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_NAME ) ;
         if ( String == e.type() )
         {
            _csName = e.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_NAME, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLoadCollectionSpace::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                        _SDB_RTNCB * rtnCB,
                                        _dpsLogWrapper *dpsCB,
                                        INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() &&
           CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         rc = SDB_RTN_COORD_ONLY ;
         goto error ;
      }

      if ( _needChangeID )
      {
         rc = rtnLoadCollectionSpace( _csName,
                                      pmdGetOptionCB()->getDbPath(),
                                      pmdGetOptionCB()->getIndexPath(),
                                      pmdGetOptionCB()->getLobPath(),
                                      pmdGetOptionCB()->getLobMetaPath(),
                                      cb, dmsCB, FALSE,
                                      &_csUniqueID, _clInfoObj ) ;
      }
      else
      {
         rc = rtnLoadCollectionSpace( _csName,
                                      pmdGetOptionCB()->getDbPath(),
                                      pmdGetOptionCB()->getIndexPath(),
                                      pmdGetOptionCB()->getLobPath(),
                                      pmdGetOptionCB()->getLobMetaPath(),
                                      cb, dmsCB, FALSE ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLoadCollectionSpace::setCSUniqueID( utilCSUniqueID csUniqueID )
   {
      _csUniqueID = csUniqueID ;
      _needChangeID = TRUE ;
   }

   void _rtnLoadCollectionSpace::setCLInfo ( const BSONObj& clInfoObj )
   {
      _clInfoObj = clInfoObj.getOwned() ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnUnloadCollectionSpace )
   _rtnUnloadCollectionSpace::_rtnUnloadCollectionSpace()
   {
   }
   _rtnUnloadCollectionSpace::~_rtnUnloadCollectionSpace()
   {
   }

   INT32 _rtnUnloadCollectionSpace::doit( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                                          _SDB_RTNCB * rtnCB,
                                          _dpsLogWrapper * dpsCB,
                                          INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() &&
           CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         rc = SDB_RTN_COORD_ONLY ;
         goto error ;
      }

      rc = rtnUnloadCollectionSpace( _csName, cb, dmsCB ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAnalyze )
   _rtnAnalyze::_rtnAnalyze()
   {
      _csname = NULL ;
      _clname = NULL ;
      _ixname = NULL ;
   }

   _rtnAnalyze::~_rtnAnalyze()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNANALYZE_INIT, "_rtnAnalyze::init" )
   INT32 _rtnAnalyze::init ( INT32 flags, INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pMatcherBuff,
                             const CHAR *pSelectBuff,
                             const CHAR *pOrderByBuff,
                             const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZE_INIT ) ;

      BOOLEAN sampleByNum = FALSE, sampleByPercent = FALSE ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e ;

         // Check collection space name
         e = matcher.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            _csname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTION, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // Check collection name
         e = matcher.getField( FIELD_NAME_COLLECTION ) ;
         if ( String == e.type() )
         {
            _clname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // Check index name
         e = matcher.getField( FIELD_NAME_INDEX ) ;
         if ( String == e.type() )
         {
            _ixname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_INDEX, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // Check mode
         e = matcher.getField( FIELD_NAME_ANALYZE_MODE ) ;
         if ( NumberInt == e.type() )
         {
            _param._mode = e.numberInt() ;
            if ( SDB_ANALYZE_MODE_SAMPLE == _param._mode ||
                 SDB_ANALYZE_MODE_FULL == _param._mode ||
                 SDB_ANALYZE_MODE_GENDFT == _param._mode ||
                 SDB_ANALYZE_MODE_RELOAD == _param._mode ||
                 SDB_ANALYZE_MODE_CLEAR == _param._mode )
            {
               /// do nothing
            }
            else
            {
               PD_LOG( PDERROR, "Value of field[%s] is invalid",
                       FIELD_NAME_ANALYZE_MODE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_ANALYZE_MODE, matcher.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // Check sample number
         e = matcher.getField( FIELD_NAME_ANALYZE_NUM ) ;
         if ( NumberInt == e.type() )
         {
            _param._sampleRecords = (UINT32) e.numberInt() ;
            if ( _param._sampleRecords > SDB_ANALYZE_SAMPLE_MAX ||
                 _param._sampleRecords < SDB_ANALYZE_SAMPLE_MIN )
            {
               PD_LOG( PDERROR, "Field[%s] %u is out of range [ %d - %d ]",
                       FIELD_NAME_ANALYZE_NUM, _param._sampleRecords,
                       SDB_ANALYZE_SAMPLE_MIN, SDB_ANALYZE_SAMPLE_MAX ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            sampleByNum = TRUE ;
         }

         // Check sample percent
         e = matcher.getField( FIELD_NAME_ANALYZE_PERCENT ) ;
         if ( NumberInt == e.type() )
         {
            _param._samplePercent = e.numberInt() ;
            if ( _param._samplePercent > 100.0 ||
                 _param._samplePercent <= 0.0 )
            {
               PD_LOG( PDERROR, "Field[%s] %.2f is out of range ( %.2f - %.2f ]",
                       FIELD_NAME_ANALYZE_PERCENT, _param._samplePercent,
                       0.0, 100.0 ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            sampleByPercent = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // Check conflicts
      if ( NULL != _csname )
      {
         if ( NULL != _clname )
         {
            PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                    FIELD_NAME_COLLECTIONSPACE, FIELD_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( NULL != _ixname )
         {
            PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                    FIELD_NAME_COLLECTIONSPACE, FIELD_NAME_INDEX ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( NULL != _ixname && NULL == _clname )
      {
         PD_LOG( PDERROR, "Field[%s] requires Field[%s]",
                 FIELD_NAME_INDEX, FIELD_NAME_COLLECTION ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_ANALYZE_MODE_GENDFT == _param._mode )
      {
         PD_CHECK( NULL != _clname, SDB_INVALIDARG, error, PDERROR,
                   "Only support generating default statistics on specified "
                   "collection or index" ) ;
      }

      if ( sampleByNum && sampleByPercent )
      {
         PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                 FIELD_NAME_ANALYZE_NUM, FIELD_NAME_ANALYZE_PERCENT ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( sampleByPercent )
      {
         _param._sampleByNum = FALSE ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNANALYZE_INIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNANALYZE_DOIT, "_rtnAnalyze::doit" )
   INT32 _rtnAnalyze::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                             INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZE_DOIT ) ;

      rc = rtnAnalyze( _csname, _clname, _ixname, _param,
                       cb, dmsCB, rtnCB, dpsCB ) ;

      if ( ( SDB_DMS_CS_NOTEXIST == rc && NULL == _csname && NULL == _clname ) ||
           ( SDB_DMS_NOTEXIST == rc && NULL == _clname ) )
      {
         // The error should be found earlier in clsShardSesssion
         // If report here, means the collection or collection space had been
         // dropped, ignore the error to avoid clsShardSession to retry
         rc = SDB_OK ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run analyze command, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNANALYZE_DOIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}

