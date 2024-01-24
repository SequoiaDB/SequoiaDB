#include "coordCommandDataSource.hpp"
#include "coordCB.hpp"
#include "msgMessage.hpp"
#include "coordDSChecker.hpp"

namespace engine
{
   static INT32 _coordDataSourceInvalidateCache( const BSONObj &cmdObject,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      CHAR *msg = NULL ;
      INT32 buffSize = 0 ;
      INT64 contextID = -1 ;
      coordCMDDataSourceInvalidator invalidator ;

      rc = msgBuildDataSourceInvalidateCacheMsg( &msg, &buffSize, cmdObject,
                                                 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build invalidate data source cache message "
                                "failed[%d]", rc ) ;

      rc = invalidator.init( sdbGetCoordCB()->getResource(), cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize invalidate data source cache "
                                "command failed[%d]", rc ) ;

      rc = invalidator.execute( (MsgHeader *)msg, cb, contextID, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute invalidate data source command "
                                "failed[%d]", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateDataSource,
                                      CMD_NAME_CREATE_DATASOURCE,
                                      FALSE ) ;
   _coordCMDCreateDataSource::_coordCMDCreateDataSource()
   {

   }

   _coordCMDCreateDataSource::~_coordCMDCreateDataSource()
   {

   }

   // Note: refer to _coordCMDCreateCataGroup::execute
   INT32 _coordCMDCreateDataSource::execute( MsgHeader *pMsg, pmdEDUCB *cb,
                                             INT64 &contextID,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *query = NULL ;
      CHAR *newMsg = NULL ;

      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL, &query,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse create data source message "
                                "failed[%d]", rc ) ;
      try
      {
         BSONObjBuilder builder ;
         BSONObj newQuery ;
         INT32 msgLen = 0 ;
         BSONObj dummyObj ;
         BSONObj dsMeta ;
         BSONElement versionEle ;
         BSONObj arguments( query ) ;
         coordDSInfoChecker dsChecker ;
         coordDSAddrChecker addrChecker ;

         rc = dsChecker.check( arguments, cb, &dsMeta ) ;
         PD_RC_CHECK( rc, PDERROR, "Check data source information failed[%d]",
                      rc ) ;

         versionEle = dsMeta.getField( FIELD_NAME_VERSION ) ;
         if ( versionEle.eoo() )
         {
            // Data source version is required, as data source message
            // reformatting replies on that for compatible reason.
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get version of data source[%s] failed[%d]",
                    arguments.getStringField( FIELD_NAME_NAME ), rc ) ;
            goto error ;
         }

         // Rebuild the option record after getting data source version.
         // Append data source version, and so on.
         builder.appendElements( arguments ) ;
         builder.append( versionEle ) ;
         newQuery = builder.done() ;

         rc = msgBuildQueryCMDMsg( &newMsg, &msgLen,
                                   CMD_ADMIN_PREFIX CMD_NAME_CREATE_DATASOURCE,
                                   newQuery, dummyObj, dummyObj, dummyObj,
                                   0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Rebuild the create data source message "
                                   "failed[%d]", rc ) ;

         rc = executeOnCataGroup( (MsgHeader *)newMsg, cb, TRUE, NULL, NULL,
                                  buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command[%s], "
                                   "rc: %d", getName(), rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( newMsg )
      {
         msgReleaseBuffer( newMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropDataSource,
                                      CMD_NAME_DROP_DATASOURCE,
                                      FALSE ) ;
   _coordCMDDropDataSource::_coordCMDDropDataSource()
   {
   }

   _coordCMDDropDataSource::~_coordCMDDropDataSource()
   {
   }

   INT32 _coordCMDDropDataSource::execute( MsgHeader *pMsg, pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *query = NULL ;

      contextID = -1 ;

      _printDebug( (const CHAR*)pMsg, getName() ) ;

      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                            &query, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract dropping data source message "
                   "failed[%d]", rc ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj queryObj( query ) ;

         rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command[%s]: "
                      "%d", getName(), rc ) ;

         rc = _coordDataSourceInvalidateCache( queryObj, cb ) ;
         PD_RC_CHECK( rc,PDERROR, "Invalidate data source cache failed[%d]",
                      rc ) ;
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

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterDataSource,
                                      CMD_NAME_ALTER_DATASOURCE,
                                      FALSE ) ;
   _coordCMDAlterDataSource::_coordCMDAlterDataSource()
   {

   }

   _coordCMDAlterDataSource::~_coordCMDAlterDataSource()
   {
   }

   INT32 _coordCMDAlterDataSource::execute( MsgHeader *pMsg, pmdEDUCB *cb,
                                            INT64& contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *query = NULL ;

      /*
       * The queryObj is in the format of
       * {
       *    "Name": <oldDSName>,
       *    "Options":
       *    {
       *       "Name": <newDSName>,
       *       "Address": <newAddress>
       *       ...
       *    }
       * }
       */

      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                            &query, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract altering data source message "
                                "failed[%d]", rc ) ;

      try
      {
         const CHAR *name = NULL ;
         const CHAR *user = NULL ;
         const CHAR *password = NULL ;
         const CHAR *address = NULL ;
         BSONElement ele ;
         BSONObj dummyObj ;
         BSONObj queryObj( query ) ;
         BSONObj optionObj = queryObj.getObjectField( FIELD_NAME_OPTIONS ) ;
         if ( optionObj.isEmpty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Alter option is empty" ) ;
            goto error ;
         }

         ele = optionObj.getField( FIELD_NAME_NAME ) ;
         if ( !ele.eoo() )
         {
            name = ele.valuestr() ;
            rc = coordDSInfoChecker::checkDSName( name ) ;
            PD_RC_CHECK( rc, PDERROR, "Data source name[%s] is invalid[%d]",
                         name ,rc ) ;
         }
         ele = optionObj.getField( FIELD_NAME_USER ) ;
         if ( !ele.eoo() )
         {
            user = ele.valuestr() ;
         }
         ele = optionObj.getField( FIELD_NAME_PASSWD ) ;
         if ( !ele.eoo() )
         {
            password = ele.valuestr() ;
         }
         ele = optionObj.getField( FIELD_NAME_ADDRESS ) ;
         if ( !ele.eoo() )
         {
            address = ele.valuestr() ;
         }

         // If the user is altering the address, user name or password of the
         // data source, need to validate the connection information.
         if ( user || password || address )
         {
            CoordDataSourcePtr dsPtr ;
            coordDSAddrChecker addrChecker ;
            const CHAR *dsName = queryObj.getStringField( FIELD_NAME_NAME ) ;
            SDB_ASSERT( ossStrlen( dsName ) > 0, "Data source name is null" ) ;

            rc = _pResource->getDSManager()->getOrUpdateDataSource( dsName,
                                                                    dsPtr,
                                                                    cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Get data source[%s] failed[%d]",
                         dsName, rc ) ;

            if ( !user )
            {
               user = dsPtr->getUser() ;
            }
            if ( !password )
            {
               password = dsPtr->getPassword() ;
            }
            if ( !address )
            {
               address = dsPtr->getAddress() ;
            }

            rc = addrChecker.check( address, user, password, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check address[%s] of data source[%s] "
                         "failed[%d]", address, dsPtr->getName(), rc ) ;
         }

         rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute alter data source command on "
                                   "catalogue group failed[%d]", rc ) ;

         rc = _coordDataSourceInvalidateCache( queryObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalidate data source cache failed[%d]",
                      rc ) ;
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

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDataSourceInvalidator,
                                      CMD_NAME_INVALIDATE_DATASOURCE_CACHE,
                                      FALSE ) ;
   _coordCMDDataSourceInvalidator::_coordCMDDataSourceInvalidator()
   {

   }

   _coordCMDDataSourceInvalidator::~_coordCMDDataSourceInvalidator()
   {

   }

   BOOLEAN _coordCMDDataSourceInvalidator::_useContext()
   {
      return FALSE ;
   }

   INT32 _coordCMDDataSourceInvalidator::_onLocalMode( INT32 flag )
   {
      return SDB_OK ;
   }

   void _coordCMDDataSourceInvalidator::_preSet( pmdEDUCB *cb,
                                                 coordCtrlParam& ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordCMDDataSourceInvalidator::_getControlMask() const
   {
      return COORD_CTRL_MASK_GLOBAL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _coordInvalidateDataSourceCache )
   _coordInvalidateDataSourceCache::_coordInvalidateDataSourceCache()
   : _dsID( UTIL_INVALID_DS_UID )
   {
   }

   _coordInvalidateDataSourceCache::~_coordInvalidateDataSourceCache()
   {
   }

   const CHAR *_coordInvalidateDataSourceCache::name()
   {
      return NAME_INVALIDATE_DATASOURCE_CACHE ;
   }

   RTN_COMMAND_TYPE _coordInvalidateDataSourceCache::type()
   {
      return CMD_INVALIDATE_DATASOURCE_CACHE ;
   }

   INT32 _coordInvalidateDataSourceCache::init( INT32 flags, INT64 numToSkip,
                                                INT64 numToReturn,
                                                const CHAR *pMatcherBuff,
                                                const CHAR *pSelectBuff,
                                                const CHAR *pOrderByBuff,
                                                const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      CoordDataSourcePtr dsPtr ;
      CoordCB *coordCB = pmdGetKRCB()->getCoordCB() ;
      coordDataSourceMgr *dsMgr = coordCB->getDSManager() ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement ele = matcher.getField( FIELD_NAME_NAME ) ;
         if ( ele.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Data source name is not specified in invalidate "
                    "cache command[%d]", rc ) ;
            goto error ;
         }

         rc = dsMgr->getDataSource( ele.valuestr(), dsPtr ) ;
         if ( rc )
         {
            // Data source with the given name dose not exist in the manager. It
            // has not been fetched from the catalogue yet. So just return OK.
            rc = SDB_OK ;
            goto done ;
         }

         _dsID = dsPtr->getID() ;
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

   INT32 _coordInvalidateDataSourceCache::doit( _pmdEDUCB *cb,
                                                _SDB_DMSCB *dmsCB,
                                                _SDB_RTNCB *rtnCB,
                                                _dpsLogWrapper *dpsCB, INT16 w,
                                                INT64 *pContextID )
   {
      if ( UTIL_INVALID_DS_UID != _dsID )
      {
         CoordCB *coordCB = pmdGetKRCB()->getCoordCB() ;
         coordDataSourceMgr *dsMgr = coordCB->getDSManager() ;
         dsMgr->removeDataSource( _dsID ) ;
      }

      return SDB_OK ;
   }                                                
}
