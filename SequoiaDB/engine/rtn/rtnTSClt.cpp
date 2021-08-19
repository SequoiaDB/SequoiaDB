#include "rtnTSClt.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "rtnCB.hpp"
#include "rtnRemoteMessenger.hpp"

namespace engine
{
   INT32 rtnTSGetCount( const rtnQueryOptions &options,
                        pmdEDUCB *cb, INT64 &count )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 length = 0 ;
      BSONObj dummyObj ;
      UINT64 sessionID = 0 ;
      MsgOpReply *reply = NULL ;
      INT32 flag = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      INT64 contextID = -1 ;
      vector<BSONObj> records ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = rtnCB->getRemoteMessenger() ;

      try
      {
         BSONObj hint = BSON( FIELD_NAME_COLLECTION << options.getCLFullName() ) ;
         rc = msgBuildQueryCMDMsg( &msg, &length,
                                 CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                                 options.getQuery(), dummyObj, dummyObj,
                                 hint, 0, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build count command message failed[%d]", rc ) ;

         rc = messenger->prepareSession( cb, sessionID ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare session failed[%d]", rc ) ;

         rc = messenger->send( sessionID, (MsgHeader *)msg, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Send message with remote messenger failed[%d]",
                     rc ) ;

         rc = messenger->receive( sessionID, cb, reply ) ;
         PD_RC_CHECK( rc, PDERROR, "Receive reply by remote messenger failed[%d]",
                     rc ) ;

         rc = msgExtractReply( (CHAR *)reply, &flag, &contextID, &startFrom,
                              &numReturned, records )  ;
         PD_RC_CHECK( rc, PDERROR, "Extract query reply failed[%d]", rc ) ;

         rc = flag ;
         if ( rc )
         {
            if ( 0 != records.size() )
            {
               PD_LOG_MSG( PDERROR, "Error returned from remote: %s",
                           records.front().toString( FALSE, TRUE ).c_str() ) ;
            }
            PD_LOG( PDERROR, "Get count from remote failed[%d]", rc ) ;
            goto error ;
         }

         {
            BSONObj result = records.front() ;
            BSONElement ele = result.getField( FIELD_NAME_TOTAL ) ;
            count = ele.numberLong() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }
}
