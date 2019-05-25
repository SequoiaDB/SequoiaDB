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
#include "aggrDef.hpp"
#include "utilCompressor.hpp"
#include "msgMessageFormat.hpp"

#if defined (_DEBUG)
#endif

using namespace bson ;
using namespace std ;

#define MAX_CL_SIZE_ALIGN_SIZE            ( 32 * 1024 * 1024 )

#define MAX_CAP_CL_SIZE                   ( OSS_SINT64_MAX >> 20 )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER_RELCMDINFO, "_rtnCmdBuilder::_releaseCmdInfo" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__REGISTER, "_rtnCmdBuilder::_register" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__INSERT, "_rtnCmdBuilder::_insert" )
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
         newCmdInfo = SDB_OSS_NEW _cmdBuilderInfo ;
         newCmdInfo->cmdName = pCmdInfo->cmdName.substr( sameNum ) ;
         newCmdInfo->createFunc = pCmdInfo->createFunc ;
         newCmdInfo->nameSize = pCmdInfo->nameSize - sameNum ;
         newCmdInfo->next = pCmdInfo->next ;
         newCmdInfo->sub = NULL ;

         pCmdInfo->next = newCmdInfo ;

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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__FIND, "_rtnCmdBuilder::_find" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDBUILDER__NEAR, "_rtnCmdBuilder::_near" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCMDASSIT__RTNCMDASSIT, "_rtnCmdAssit::_rtnCmdAssit" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_INIT, "_rtnCreateCollection::init" )
   INT32 _rtnCreateCollection::init( INT32 flags, INT64 numToSkip,
                                     INT64 numToReturn,
                                     const CHAR * pMatcherBuff,
                                     const CHAR * pSelectBuff,
                                     const CHAR * pOrderByBuff,
                                     const CHAR * pHintBuff)
   {
      INT32 rc = SDB_OK ;
      BOOLEAN enSureIndex = TRUE ;
      BOOLEAN isCompressed = FALSE ;
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
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_ENSURE_SHDINDEX,
                                 enSureIndex ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         enSureIndex = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Field[%s] value is error in obj[%s]",
                   FIELD_NAME_ENSURE_SHDINDEX, matcher.toString().c_str() ) ;
      if ( enSureIndex )
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
      rtnGetBooleanElement ( matcher, FIELD_NAME_COMPRESSED,
                             isCompressed ) ;
      if ( isCompressed )
      {
         _attributes |= DMS_MB_ATTR_COMPRESSED ;
      }

      rtnGetBooleanElement ( matcher, FIELD_NAME_STRICTDATAMODE,
                             strictDataMode ) ;
      if ( strictDataMode )
      {
         _attributes |= DMS_MB_ATTR_STRICTDATAMODE ;
      }

      rc = rtnGetStringElement( matcher, FIELD_NAME_COMPRESSIONTYPE,
                                &compressionType ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         if ( isCompressed )
         {
            _compressorType = UTIL_COMPRESSOR_SNAPPY ;
         }
      }
      else
      {
         if ( SDB_OK == rc )
         {
            if ( !isCompressed )
            {
               PD_LOG( PDERROR, "Compression type is specified while "
                       "Compressed option is false" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

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
         else
         {
            PD_LOG( PDERROR, "Failed to get CompressionType, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = rtnGetBooleanElement( matcher, FIELD_NAME_AUTO_INDEX_ID,
                                 autoIndexId ) ;
      if ( SDB_OK == rc && !autoIndexId )
      {
         _attributes |= DMS_MB_ATTR_NOIDINDEX ;
      }

      rc = rtnGetBooleanElement( matcher, FIELD_NAME_CAPPED, capped ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         capped = FALSE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Field[%s] value error in obj[%s]",
                   FIELD_NAME_CAPPED, matcher.toString().c_str() ) ;

      if ( capped )
      {
         INT64 maxSize = 0 ;
         INT64 maxRecNum = 0 ;
         BOOLEAN overwrite = FALSE ;
         BSONObjBuilder builder ;

         if ( isCompressed || autoIndexId )
         {
            PD_LOG( PDERROR, "Option Sharding/Compress/Index is not compatible "
                    "with Capped" ) ;
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
         if ( maxSize <= 0 || maxSize > MAX_CAP_CL_SIZE )
         {
            PD_LOG( PDERROR, "Invalid Size[ %lld ] when creating capped "
                    "collection", maxSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         maxSize = ossRoundUpToMultipleX( maxSize << 20,
                                          MAX_CL_SIZE_ALIGN_SIZE ) ;
         builder.append( FIELD_NAME_SIZE, maxSize ) ;

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
      _clean( pmdGetThreadEDUCB(), pmdGetKRCB()->getDMSCB(),
              pmdGetKRCB()->getDPSCB() ) ;
      goto done ;
   }

   const CHAR *_rtnCreateCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL_DOIT, "_rtnCreateCollection::doit" )
   INT32 _rtnCreateCollection::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                      INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECL_DOIT ) ;

      rc = rtnCreateCollectionCommand ( _collectionName, _shardingKey,
                                        _attributes, cb, dmsCB, dpsCB,
                                        _compressorType, 0, FALSE,
                                        ( _extOptions.isEmpty() ?
                                         NULL : &_extOptions ) ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         CHAR szTmp[ 50 ] = { 0 } ;
         mbAttr2String( _attributes, szTmp, sizeof(szTmp) - 1 ) ;
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECL__CLEAN, "_rtnCreateCollection::_clean" )
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
            PD_LOG( PDERROR, "Drop new created collection space[%s] failed, "
                    "rc: %d", csName.c_str(), rc ) ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNCREATECL__CLEAN ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateCollectionspace)
   _rtnCreateCollectionspace::_rtnCreateCollectionspace ()
   :_spaceName ( NULL ),
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATECS_DOIT, "_rtnCreateCollectionspace::doit" )
   INT32 _rtnCreateCollectionspace::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                           INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATECS_DOIT ) ;

      rc = rtnCreateCollectionSpaceCommand ( _spaceName, cb, dmsCB,
                                             dpsCB, _pageSize,
                                             _lobPageSize, _storageType ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_INIT, "_rtnCreateIndex::init" )
   INT32 _rtnCreateIndex::init ( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff)
   {
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      BSONObj hint ( pHintBuff ) ;

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

      rc = _validateDef( _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to validate index definition: %s, rc: %d",
                  _index.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( hint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         rc = rtnGetIntElement( hint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufferSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get index sort buffer, hint: %s",
                     hint.toString().c_str() ) ;
            goto error ;
         }

         if ( _sortBufferSize < 0 )
         {
            PD_LOG ( PDERROR, "invalid index sort buffer size: %d", _sortBufferSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX_DOIT, "_rtnCreateIndex::doit" )
   INT32 _rtnCreateIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCREATEINDEX_DOIT ) ;
      BOOLEAN isSys = FALSE ;

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
                                   dmsCB, dpsCB, isSys, _sortBufferSize ) ;

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEINDEX__VALIDATEDEF, "_rtnCreateIndex::_validateDef" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCL_DOIT, "" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPCS_DOIT, "_rtnDropCollectionspace::doit" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_INIT, "_rtnDropIndex::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPINDEX_DOIT, "_rtnDropIndex::doit" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_INIT, "_rtnGet::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGET_DOIT, "_rtnGet::doit" )
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
         _options.setHint( _options.getSelector() ) ;
      }

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                              *pContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Create context failed, rc: %d", rc ) ;

      rc = context->open( BSONObj(), BSONObj(), -1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   *pContextID, rc ) ;

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

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnRenameCollection)
   _rtnRenameCollection::_rtnRenameCollection ()
      :_oldCollectionName ( NULL ), _newCollectionName ( NULL ), _csName( NULL )
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
                                    &_oldCollectionName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[oldname]" ) ;
            goto error ;
         }
         rc = rtnGetStringElement ( arg, FIELD_NAME_NEWNAME,
                                    &_newCollectionName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[newname]" ) ;
            goto error ;
         }

         _fullCollectionName = _csName ;
         _fullCollectionName += "." ;
         _fullCollectionName += _oldCollectionName ;
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
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      BOOLEAN dmsLock = FALSE ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto error ;
      }
      dmsLock = TRUE ;

      rc = rtnCollectionSpaceLock ( _csName, dmsCB, FALSE, &su, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get collection space:%s", _csName ) ;
         goto error ;
      }

      rc = su->data()->renameCollection ( _oldCollectionName,
                                          _newCollectionName,
                                          cb, dpsCB ) ;
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _oldCollectionName, rc,
                           "NewCollectionName:%s",
                           _newCollectionName ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to rename collection from %s to %s",
            _oldCollectionName, _newCollectionName ) ;
         goto error ;
      }

   done:
      if ( suID != DMS_INVALID_CS )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( dmsLock )
      {
         dmsCB->writeDown ( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNRENAMECL_DOIT, rc ) ;
      return rc ;
   error:
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
      BOOLEAN dmsLock = FALSE ;

      rc = dmsCB->writable ( cb ) ;
      if ( rc )
      {
         goto error ;
      }
      dmsLock = TRUE ;

      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( _oldName, cb ) ;
      }

      rc = dmsCB->renameCollectionSpace( _oldName, _newName, cb, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to rename collectionspace from %s to %s",
                  _oldName, _newName ) ;
         goto error ;
      }
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CS,
                           _oldName, rc,
                           "NewCollectionSpaceName:%s",
                           _newName ) ;
      }

   done:
      if ( dmsLock )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNRENAMECS_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREORG_INIT, "_rtnReorg::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_INIT, "_rtnTest::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTEST_DOIT, "_rtnTest::doit" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_INIT, "_rtnSetPDLevel::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETPDLEVEL_DOIT, "_rtnSetPDLevel::doit" )
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
      optCB->toBSON( cfgObj ) ;

      PD_LOG( PDEVENT, "Reload config succeed. All configs: %s",
              cfgObj.toString().c_str() ) ;

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
      BSONObj options = BSONObj( pMatcherBuff ) ;
      _newCfgObj = options.getObjectField( FIELD_NAME_CONFIGS ) ;
      _newCfgObj.getOwned() ;

      return SDB_OK ;
   }

   INT32 _rtnUpdateConfig::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      BSONObj returnObj ;
      string returnStr;
      INT32 rebootCount = 0 ;
      BOOLEAN rebootFirstEntry  = TRUE ;
      INT32 forbidCount = 0 ;
      BOOLEAN forbidFirstEntry  = TRUE ;
      BSONElement rebootEle ;
      BSONElement forbidEle ;

      rc = optCB->changeConfig( _newCfgObj, returnObj ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Update config[%s] failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      try
      {
         rebootEle = returnObj.getField( "Reboot" ) ;
         if ( Array == rebootEle.type() )
         {
            BSONArray array = BSONArray( rebootEle.embeddedObject() ) ;
            BSONObjIterator iter( array );
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == rebootFirstEntry )
                  {
                     returnStr += "config" ;
                     returnStr += " '" ;
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
            BSONArray array = BSONArray( forbidEle.embeddedObject() ) ;
            BSONObjIterator iter( array );
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == forbidFirstEntry )
                  {
                     returnStr += " config" ;
                     returnStr += " '" ;
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
         PD_LOG( PDWARNING, "Exception during updateConf info parsing: %s",
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
      string returnStr;
      INT32 rebootCount = 0 ;
      BOOLEAN rebootFirstEntry  = TRUE ;
      INT32 forbidCount = 0 ;
      BOOLEAN forbidFirstEntry  = TRUE ;
      BSONElement rebootEle ;
      BSONElement forbidEle ;

      rc = optCB->changeConfig( _newCfgObj, returnObj, TRUE ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Delete config[%s] failed, rc: %d",
                 _newCfgObj.toString().c_str(), rc ) ;
         goto error ;
      }

      try
      {
         rebootEle = returnObj.getField( "Reboot" ) ;
         if ( Array == rebootEle.type() )
         {
            BSONArray array = BSONArray( rebootEle.embeddedObject() ) ;
            BSONObjIterator iter( array );
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == rebootFirstEntry )
                  {
                     returnStr += "config" ;
                     returnStr += " '" ;
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
            BSONArray array = BSONArray( forbidEle.embeddedObject() ) ;
            BSONObjIterator iter( array );
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               if ( String == ele.type() )
               {
                  if ( TRUE == forbidFirstEntry )
                  {
                     returnStr += " config" ;
                     returnStr += " '" ;
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
         PD_LOG( PDWARNING, "Exception during deleteConf info parsing: %s",
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_INIT, "_rtnTraceStart::init" )
   INT32 _rtnTraceStart::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_INIT ) ;
      try
      {
         BSONObj arg ( pMatcherBuff );
         BSONElement eleTID  = arg.getField ( FIELD_NAME_THREADS ) ;
         BSONElement eleSize = arg.getField ( FIELD_NAME_SIZE ) ;
         UINT32 one = 1 ;
         BSONElement eleComp = arg.getField( FIELD_NAME_COMPONENTS );
         BSONElement eleBreakPoint = arg.getField(FIELD_NAME_BREAKPOINTS);
         if ( eleComp.type() == Array )
         {
            _mask = 0 ;
            BSONObjIterator it ( eleComp.embeddedObject() ) ;
            if ( !it.more () )
            {
               for( INT32 i = 0; i < _pdTraceComponentNum; ++i )
               {
                  _mask |= one << i ;
               }
            }
            else
            {
               while ( it.more() )
               {
                  BSONElement ele = it.next() ;
                  if ( ele.type() == String )
                  {
                     for ( INT32 i = 0; i < _pdTraceComponentNum; ++i)
                     {
                        if ( ossStrcmp ( pdGetTraceComponent(i),
                                     ele.valuestr() ) == 0 )
                        {
                           _mask |= one<<i;
                        }
                     } // for
                  } // if ( ele.type() == String )
               } // while ( it.more() )
            }
         } // if ( eleComp.type() == Array )
#if defined (SDB_ENGINE)
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
                        _funcCode.push_back ( i ) ;
                     } // if( 0 == ossStrcmp( funcName, eleStr ) )
                  } // for( UINT64 i = 0; i < pdGetTraceFunctionListNum(); i++ )
               } // if( ele.type() == String )
            } // while( it.more() )
         }
#endif
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
            } // while ( it.more () )
         } // if ( eleTID.type() == Array )
         if ( eleSize.isNumber() )
         {
            _size = (UINT32)eleSize.numberLong() ;
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



   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTART_DOIT, "_rtnTraceStart::doit" )
   INT32 _rtnTraceStart::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACESTART_DOIT ) ;
      rc = sdbGetPDTraceCB()->start ( (UINT64)_size, _mask, &_funcCode, &_tid ) ;
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_INIT, "_rtnTraceResume::init" )
   INT32 _rtnTraceResume::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                const CHAR * pMatcherBuff, const CHAR * pSelectBuff,
                                const CHAR * pOrderByBuff, const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_INIT ) ;
      PD_TRACE_EXITRC ( SDB__RTNTRACERESUME_INIT, rc ) ;
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACERESUME_DOIT, "_rtnTraceResume::doit" )
   INT32 _rtnTraceResume::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNTRACERESUME_DOIT ) ;
      pdTraceCB *pdTraceCB = sdbGetPDTraceCB() ;
      pdTraceCB->removeAllBreakPoint () ;
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_INIT, "_rtnTraceStop::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTOP_DOIT, "_rtnTraceStop::doit" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_INIT, "_rtnTraceStatus::init" )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTRACESTATUS_DOIT, "_rtnTraceStatus::doit" )
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

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnLoad)
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
   :_contextID( -1 ),
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
         BSONElement ele = _query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid collection name in condition:%s",
                    _query.toString( FALSE, TRUE ).c_str() ) ;
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

      SDB_ASSERT( NULL != context, "can not be null" ) ;
      rc = context->open( _query, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open list lob context:%d", rc ) ;
         goto error ;
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

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNSETSESSATTR_INIT, "_rtnSetSessionAttr::init" )
   INT32 _rtnSetSessionAttr::init( INT32 flags, INT64 numToSkip,
                                   INT64 numToReturn,
                                   const CHAR *pMatcherBuff,
                                   const CHAR *pSelectBuff,
                                   const CHAR *pOrderByBuff,
                                   const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETSESSATTR_INIT ) ;

      try
      {
         BSONObj property( pMatcherBuff ) ;
         rtnSessionProperty sessProperty ;

         rc = sessProperty.parseProperty( property ) ;
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
      PD_TRACE_EXITRC( SDB__RTNSETSESSATTR_INIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _rtnSetSessionAttr::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                   _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                   INT16 w, INT64 *pContextID )
   {
      return SDB_OK ;
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

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNTRUNCATE_DOIT, ""_rtnTruncate::doit" )
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

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNPOP_DOIT, ""_rtnPop::doit" )
   INT32 _rtnPop::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                         _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                         INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNPOP_DOIT ) ;

      rc = rtnPopCommand( _fullName, _logicalID, cb,
                          dmsCB, dpsCB, w, _direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pop record from collection[%s], "
                   "rc: %d", _fullName, rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNPOP_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAlterCollection )
   _rtnAlterCollection::_rtnAlterCollection()
   {

   }

   _rtnAlterCollection::~_rtnAlterCollection()
   {

   }

   const CHAR *_rtnAlterCollection::collectionFullName()
   {
      if ( !_alterObj.isEmpty() )
      {
         return _alterObj.getField( FIELD_NAME_NAME ).valuestrsafe() ;
      }
      else
      {
         return RTN_ALTER_TYPE_CL == _runner.getJob().getType() &&
                NULL != _runner.getJob().getName() ?
                _runner.getJob().getName() : NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION_INIT, "_rtnAlterCollection::init" )
   INT32 _rtnAlterCollection::init( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                                    const CHAR *pMatcherBuff,
                                    const CHAR *pSelectBuff,
                                    const CHAR *pOrderByBuff,
                                    const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION_INIT ) ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         if ( obj.getField( FIELD_NAME_VERSION ).eoo() )
         {
            _alterObj = BSONObj( pMatcherBuff ) ;
            BSONElement options ;
            BSONElement clName = _alterObj.getField( FIELD_NAME_NAME ) ;
            if ( String != clName.type() )
            {
               PD_LOG( PDERROR, "invalid alter object:%s",
                       _alterObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            options = _alterObj.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object != options.type() )
            {
               PD_LOG( PDERROR, "invalid alter object:%s",
                       _alterObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            rc = _runner.init( BSONObj( pMatcherBuff ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to init rpc runner:%d", rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION_DOIT, "_rtnAlterCollection::doit" )
   INT32 _rtnAlterCollection::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                    INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION_DOIT ) ;
      if ( _alterObj.isEmpty() )
      {
         rc = _runner.run( cb, dpsCB ) ;

         if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
         {
            PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                              _runner.getJob().getName(), rc,
                              "Option:%s",
                              _runner.getJob().getJobObj().toString().c_str() ) ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to run alter command:%d", rc ) ;
            goto error ;
         }
      }
      else if ( getFromService() != CMD_SPACE_SERVICE_SHARD )
      {
         rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
         PD_LOG( PDERROR, "this request should be from shard port" ) ;
         goto error ;
      }
      else
      {
         rc = _handleOldVersion( cb, dmsCB, rtnCB,
                                 dpsCB, w, pContextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to alter collection:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION, "_rtnAlterCollection::_handleOldVersion" )
   INT32 _rtnAlterCollection::_handleOldVersion( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION ) ;
      BSONObj idxDef ;
      BSONObj options ;
      BSONElement ensureIndex  ;
      BSONElement shardingKey ;
      options = _alterObj.getField( FIELD_NAME_OPTIONS ).embeddedObject() ;
      shardingKey = options.getField( FIELD_NAME_SHARDINGKEY ) ;

      if ( Object != shardingKey.type() )
      {
         PD_LOG( PDDEBUG, "no sharding key in the alter object, do noting." ) ;
         goto done ;
      }

      ensureIndex = options.getField( FIELD_NAME_ENSURE_SHDINDEX ) ;
      if ( Bool == ensureIndex.type() &&
           !ensureIndex.Bool() )
      {
         PD_LOG( PDDEBUG, "ensureShardingIndex is false, do nothing." ) ;
         goto done ;
      }

      idxDef = BSON( IXM_FIELD_NAME_KEY << shardingKey.embeddedObject()
                     << IXM_FIELD_NAME_NAME << IXM_SHARD_KEY_NAME
                     << "v"<<0 ) ;

      rc = rtnCreateIndexCommand( collectionFullName(),
                                  idxDef,
                                  cb, dmsCB, dpsCB, TRUE ) ;
      if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create sharding key index:%d", rc ) ;
         goto error ;
      }
      else
      {
         /*
         the version which is from coord has been updated.
         we override the interface collectionFullName(). so
         the local catalog info will be autoly updated before
         this function is called.

         catalog info has been updated, clear local's info.
          it will download the last info when next request comes.
         catAgent *catAgent = sdbGetShardCB()->getCataAgent() ;
         catAgent->lock_w() ;
         catAgent->clear( collectionFullName() ) ;
         catAgent->release_w() ;
          notify other secondary nodes to clear catalog info.
         sdbGetClsCB()->invalidateCata( collectionFullName() ) ;
        */
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERCOLLECTION__HANDLEOLDVERSION, rc ) ;
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
   {
      _csName = NULL ;
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
      return rtnLoadCollectionSpace( _csName,
                                     pmdGetOptionCB()->getDbPath(),
                                     pmdGetOptionCB()->getIndexPath(),
                                     pmdGetOptionCB()->getLobPath(),
                                     pmdGetOptionCB()->getLobMetaPath(),
                                     cb, dmsCB, FALSE ) ;
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
      return rtnUnloadCollectionSpace( _csName, cb, dmsCB ) ;
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

