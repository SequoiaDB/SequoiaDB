#include "client.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "msg.hpp"
#include "pmd.hpp"


//#include "common.hpp"
#include <pthread.h>
#include <stdlib.h>
#include <boost/thread.hpp>
#include <string>
#include <unistd.h>

using namespace engine;

using bson::BSONObj;


using namespace std ;
using namespace boost ;
using namespace sdbclient ;

static CHAR *pLocalhost ;// = "localhost" ;
static const CHAR *pPort = "11810" ;
static const CHAR *pUsr = "sdbadmin" ;
static const CHAR *pPasswd = "sdbadmin" ;

//static const CHAR *


void queryThread( const CHAR *pClName, const CHAR *pFiledName, 
                       const CHAR *pOp, const INT32 valInt ) ;



INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   INT32 threadNum ;
   INT32 fieldValInt ;
   INT32 fieldVal ;
   std::string opStr = "$";
   //const CHAR *op[] = {"$et", "$gt", "$lt", }; 
   if( argc != 8 )
   {
      cout << "Format Wrong ! ./ts.out threadNum1 fieldName2 fieldValInt3 "
           << "operator4 collectionName5 sdbIP6 logFilePath7" << endl ;
      goto done ;
   }
	sdbEnablePD(argv[7]) ;
	setPDLevel( PDINFO ) ;
   threadNum = ossAtoi( argv[1] ) ;
   fieldValInt = ossAtoi( argv[3] ) ;
   pLocalhost = argv[6] ;
   opStr += argv[4] ;
   while( threadNum )
   {
      if( 0 == fieldValInt )
      {
         srand(time(NULL)) ;
         fieldVal = rand() ;
      }
      else
      {
         fieldVal = fieldValInt + threadNum ;
      }
      boost::thread( boost::bind( &queryThread, argv[5], argv[2], opStr.c_str(), 
                     fieldVal ) ).detach() ;
      threadNum-- ;
   }

   pause();
   
done:
   return rc;
}


void queryThread( const CHAR *pClName, const CHAR *pFiledName, 
                       const CHAR *pOp, const INT32 valInt )
{
   INT32 rc = SDB_OK ;
   sdb db ;
   //sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   BSONObj cond = BSON( pFiledName << BSON( pOp << valInt ) ) ;
   BSONObj defaultObj;
   BSONObj obj ;
   

   UINT64 tid = pthread_self();
   UINT64 qCount = 0;
   UINT64 resultCount = 0;

   rc = db.connect(pLocalhost, pPort, pUsr, pPasswd);
   if(SDB_OK != rc)
   {
      //cout << "sdb connect failed, tid=" << tid << endl ;
      PD_LOG( PDINFO, "sdb connect failed, tid=%lld", tid ) ;
      goto done ;
   }
   
  
   rc = db.getCollection( pClName, cl ) ;
   if(SDB_OK != rc)
   {
      //cout << "sdb getCollection " << pClName << " failed, tid=" << tid << endl ;
      PD_LOG( PDINFO, "sdb getCollection failed, tid=%lld", tid ) ;
      goto done ;
   }
   
   cout << "###start query thread : " << tid << endl;
   PD_LOG( PDINFO, "###start query thread : tid=%lld", tid ) ;
   
   while(1)
   {
      qCount++ ;
      resultCount = 0;
      sdbCursor cursor ;
      rc = cl.query( cursor, cond, defaultObj, defaultObj, defaultObj, 0, -1, FLG_QUERY_PARALLED);
      if(SDB_OK != rc)
      {
         //cout << qCount << " query failed, tid=" << tid << endl;
         PD_LOG( PDINFO, "qCount:%lld, query failed, tid=%lld", qCount, tid ) ;
      }
      else
      {
         while( !( rc=cursor.next( obj ) ) )
         {
            resultCount++;
         }
         
         cout << qCount << " query successed, tid=" << tid << ", resultCount=" 
              << resultCount << endl;
              
         PD_LOG( PDINFO, "qCount:%lld, query successed, resultCount=%lld, tid=%lld", qCount, resultCount, tid ) ;
      }
   }

done:
   return ;
}
