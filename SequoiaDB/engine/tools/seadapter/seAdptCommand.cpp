#include "seAdptCommand.hpp"
#include "msgDef.hpp"
#include "ossUtil.hpp"
#include "seAdptMgr.hpp"
#include "seAdptKeyword.hpp"
#include "utilESCltMgr.hpp"

namespace seadapter
{
   _seAdptGetCount::_seAdptGetCount()
   : _clFullName( NULL ),
     _indexID( SEADPT_INVALID_IMID )
   {
   }

   INT32 _seAdptGetCount::init( INT32 flags,
                                INT64 numToSkip,
                                INT64 numToReturn,
                                const CHAR *matcherBuff,
                                const CHAR *selectorBuff,
                                const CHAR *orderByBuff,
                                const CHAR *hintBuff )
   {
      INT32 rc = SDB_OK ;
      try
      {
         map<string, UINT16> idxNameMap ;
         BSONObj hint( hintBuff ) ;
         BSONObj matcher( matcherBuff ) ;
         // Collection name is stored in the hint.
         _clFullName = hint.getStringField( FIELD_NAME_COLLECTION ) ;
         if ( 0 == ossStrlen( _clFullName ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Collection name is invalid in the message" ) ;
            goto error ;
         }

         sdbGetSeAdapterCB()->getIdxMetaMgr()->getIdxNamesByCL( _clFullName,
                                                               idxNameMap ) ;
         if ( 0 == idxNameMap.size() )
         {
            rc = SDB_RTN_INDEX_NOTEXIST ;
            PD_LOG( PDERROR, "No text index found for collection[%s]",
                  _clFullName ) ;
            goto error ;
         }

         _condition = matcher.firstElement().Obj().firstElement().Obj() ;
         _indexID = idxNameMap.begin()->second ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpeced exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptGetCount::doit( pmdEDUCB *cb, utilCommObjBuff &objBuff )
   {
      INT32 rc = SDB_OK ;
      UINT64 totalCount = 0 ;
      seIdxMetaContext *imContext = NULL ;
      utilESClt *client = NULL ;
      utilESCltMgr *esCltMgr = utilGetESCltMgr() ;
      seIdxMetaMgr *idxMetaMgr = sdbGetSeAdapterCB()->getIdxMetaMgr() ;

      rc = esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      rc = idxMetaMgr->getIMContext( &imContext, _indexID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get index meta context failed[%d]", rc ) ;

      rc = client->getDocCount( imContext->meta()->getESIdxName(),
                                imContext->meta()->getESTypeName(),
                                _condition.toString(false, true).c_str(),
                                totalCount ) ;
      esCltMgr->releaseClient( client ) ;
      client = NULL ;
      PD_RC_CHECK( rc, PDERROR, "Get document count failed[%d]", rc ) ;

      // If it's match_all, we need to filter out the SDBCOMMIT record.
      if ( _isMatchAll() && totalCount > 0 )
      {
         --totalCount ;
      }
      rc = objBuff.appendObj( BSON( FIELD_NAME_TOTAL << (INT64)totalCount ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Append result to buffer failed[%d]", rc ) ;

   done:
      if ( client )
      {
         esCltMgr->releaseClient( client ) ;
      }
      idxMetaMgr->releaseIMContext( imContext ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _seAdptGetCount::_isMatchAll()
   {
      BOOLEAN result = FALSE ;
      BSONObjIterator itr( _condition ) ;
      BSONElement queryEle = itr.next() ;
      if ( queryEle.eoo() ||
          ( 0 != ossStrcmp( queryEle.fieldName(), KEYWORD_QUERY ) ) ||
          itr.more() )
      {
         goto done ;
      }

      {
         BSONObj matchObj = queryEle.Obj() ;
         if ( 0 == ossStrcmp( matchObj.firstElementFieldName(),
                              KEYWORD_MATCH_ALL ) )
         {
            result = TRUE ;
         }
      }

   done:
      return result ;
   }

   BOOLEAN seAdptIsCommand( const CHAR *name )
   {
      return ( name && '$' == name[0] ) ;
   }

   INT32 seAdptGetCommand( const CHAR *name, seAdptCommand *&command )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( name, CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT ) )
      {
         command = SDB_OSS_NEW seAdptGetCount() ;
         if ( !command )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for command failed" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unkown command: %s", name ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void seAdptReleaseCommand( seAdptCommand *&command )
   {
      if ( command )
      {
         SDB_OSS_DEL command ;
         command = NULL ;
      }
   }
}
