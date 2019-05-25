#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(sdb,sdbGetSnapshot_SDB_SANP_CONTEXTS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_CONTEXTS,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) << "host is: " << HOST << " , server is: "
                              << SERVER1 << " , user is: " << USER
                              << " , passwd is: " << PASSWD ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_CONTEXTS,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_CONTEXTS,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}


TEST(sdb,sdbGetSnapshot_SDB_SNAP_CONTEXTS_CURRENT)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_CONTEXTS_CURRENT,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_CONTEXTS_CURRENT,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_CONTEXTS_CURRENT,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}


TEST(sdb,sdbGetSnapshot_SDB_SNAP_SESSIONS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_SESSIONS,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_SESSIONS,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_SESSIONS,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetSnapshot_SDB_SNAP_SESSIONS_CURRENT)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_SESSIONS_CURRENT,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_SESSIONS_CURRENT,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_SESSIONS_CURRENT,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetSnapshot_SDB_SNAP_COLLECTIONS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_COLLECTIONS,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_COLLECTIONS,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_COLLECTIONS,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetSnapshot_SDB_SNAP_COLLECTIONSPACES)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_COLLECTIONSPACES,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_COLLECTIONSPACES,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetSnapshot_SDB_SNAP_DATABASE)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_DATABASE,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_DATABASE,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_DATABASE,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetSnapshot_SDB_SNAP_SYSTEM)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetSnapshot( db, SDB_SNAP_SYSTEM,
                           NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( cdb, SDB_SNAP_SYSTEM,
                           NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetSnapshot( ddb, SDB_SNAP_SYSTEM,
                           NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}
/*

TEST(sdb,sdbResetSnapshot)
{
   ASSERT_TRUE( 1==0 ) ;
   sdbConnectionHandle connection = 0 ;
   INT32 rc                       = SDB_OK ;

   bson condition ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbResetSnapshot( connection, &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;
}
*/

TEST(sdb,sdbGetList_SDB_LIST_CONTEXTS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_CONTEXTS,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_CONTEXTS,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_CONTEXTS,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_CONTEXTS_CURRENT)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_CONTEXTS_CURRENT,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_CONTEXTS_CURRENT,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_CONTEXTS_CURRENT,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_SESSIONS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_SESSIONS,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_SESSIONS,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_SESSIONS,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_SESSIONS_CURRENT)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_SESSIONS_CURRENT,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_SESSIONS_CURRENT,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_SESSIONS_CURRENT,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_COLLECTIONS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_COLLECTIONS,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_COLLECTIONS,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_COLLECTIONS,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_COLLECTIONSPACES)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_COLLECTIONSPACES,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_COLLECTIONSPACES,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_COLLECTIONSPACES,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_STORAGEUNITS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_STORAGEUNITS,
                       NULL, NULL, NULL, &cursor1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = sdbConnect ( HOST, SERVER1, USER, PASSWD, &cdb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect ( HOST, SERVER2, USER, PASSWD, &ddb ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( cdb, SDB_LIST_STORAGEUNITS,
                       NULL, NULL, NULL, &cursor2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetList( ddb, SDB_LIST_STORAGEUNITS,
                       NULL, NULL, NULL, &cursor3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      goto exit ;
   }
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCursor ( cursor1 ) ;
   sdbReleaseCursor ( cursor2 ) ;
   sdbReleaseCursor ( cursor3 ) ;
done:
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
   return ;
exit:
   sdbDisconnect ( cdb ) ;
   sdbDisconnect ( ddb ) ;
   sdbReleaseConnection ( cdb ) ;
   sdbReleaseConnection ( ddb ) ;
   goto done ;
}

TEST(sdb,sdbGetList_SDB_LIST_GROUPS)
{
   sdbConnectionHandle db         = 0 ;
   sdbConnectionHandle cdb        = 0 ;
   sdbConnectionHandle ddb        = 0 ;
   sdbCursorHandle cursor         = 0 ;
   sdbCursorHandle cursor1        = 0 ;
   sdbCursorHandle cursor2        = 0 ;
   sdbCursorHandle cursor3        = 0 ;
   INT32 rc                       = SDB_OK ;

   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetList( db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      rc = sdbGetList( db, SDB_LIST_GROUPS,
                       NULL, NULL, NULL, &cursor ) ;
      ASSERT_EQ( SDB_RTN_COORD_ONLY, rc ) ;
   } // standalone mode
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   sdbReleaseCursor ( cursor ) ;
   sdbDisconnect ( db ) ;
   sdbReleaseConnection ( db ) ;
}

