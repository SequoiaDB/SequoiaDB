/**************************************************************
 * @Description: common functions of driver test
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#ifndef TESTCOMMON_HPP__
#define TESTCOMMON_HPP__

#include "client.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace sdbclient ; 
using namespace std ;

#define CHECK_RC( expRc, actRc, fmt, ... ) \
do { \
   if( expRc != actRc ) \
   { \
      printMsg( fmt, ##__VA_ARGS__ ) ; \
      cout << endl ; \
      goto error ; \
   } \
} while( 0 ) ;

#define SDB_TEST_ERROR -10000
#define MAX_NAME_SIZE  127

void printMsg( const CHAR* fmt, ... ) ;

// create cs cl, ReplSize: 0
INT32 createNormalCsCl( sdb& db, sdbCollectionSpace& cs, sdbCollection& cl,
                        const CHAR* csName, const CHAR* clName ) ;

void ossSleep( INT32 milliSeconds ) ;

BOOLEAN isStandalone( sdb& db ) ;

INT32 getClGroups( sdb& db, const CHAR* clFullName, vector<string>& groups ) ;

INT32 getGroups( sdb& db, vector<string>& groups ) ;

INT32 getGroupNodes( sdb& db, const CHAR* rgName, vector<string>& nodes ) ;

INT32 getLocalHost( CHAR hostName[], INT32 len ) ;

INT32 getDBHost( sdb& db, CHAR hostName[], INT32 len ) ;

INT32 getCurrentLsn( sdb& db, UINT64* offset, UINT32* version ) ;

INT32 waitSync( sdb& db, const CHAR* rgName ) ;

#endif // TESTCOMMON_HPP__
