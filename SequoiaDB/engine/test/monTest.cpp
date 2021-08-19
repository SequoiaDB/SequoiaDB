#include "monMgr.hpp"
#include "gtest/gtest.h"
using namespace engine;


class monitorManagerTest : public ::testing::Test
{
public:

   monitorManagerTest() {
       // initialization code here
   }

   void SetUp( ) {
       // code here will execute just before the test ensues
   }

   void TearDown( ) {
       // code here will be called just after the test completes
       // ok to through exceptions from here if need be
   }
} ;


class monitorLockTest : public ::testing::Test
{
public:
    monitorLockTest () {

    }

    void SetUp( ) {
        // code here will execute just before the test ensues
    }

    void TearDown( ) {
        // code here will be called just after the test completes
        // ok to through exceptions from here if need be
    }

};

//the design of monMonitorManager has been changed including the input requirement of
//setMonitorStatus. The following tests should be reviewed and updated.
/*
// Test register object
TEST (monitorManagerTest, registerObj1)
{
   monMonitorManager mgr ;

   // Turn on monitoring
   mgr.setMonitorStatus( MON_CLASS_QUERY, TRUE ) ;

   monClassQuery *obj = mgr.registerMonitorObject<monClassQuery>() ;

   ASSERT_NE( obj, (monClassQuery*)NULL );
}

// Test register object when object class monitoring is off
TEST (monitorManagerTest, registerObj2)
{
   monMonitorManager mgr ;

   // Turn off monitoring
   mgr.setMonitorStatus( MON_CLASS_QUERY, FALSE ) ;

   monClassQuery *obj = mgr.registerMonitorObject<monClassQuery>() ;

   ASSERT_EQ( obj, (monClassQuery*)NULL );
}

// Test scanning objects
TEST (monitorManagerTest, scanObj1)
{
   monMonitorManager mgr ;

   // Turn on monitoring
   mgr.setMonitorStatus(MON_CLASS_QUERY, TRUE) ;

   monClassQuery *obj = mgr.registerMonitorObject<monClassQuery>() ;
   monClassQuery *obj2 = mgr.registerMonitorObject<monClassQuery>() ;

   monClassReadScanner *scanner = mgr.getReadScanner(MON_CLASS_QUERY, MON_CLASS_ACTIVE_LIST);
   monClassQuery *retObj = (monClassQuery*)scanner->getNext();

   ASSERT_EQ(retObj, obj2) ;

   retObj = (monClassQuery*)scanner->getNext() ;

   ASSERT_EQ(retObj, obj) ;

   retObj = (monClassQuery*)scanner->getNext() ;

   ASSERT_EQ( retObj, (monClassQuery*)NULL ) ;

   delete ( scanner ) ;
}

// Test scanning empty active list
TEST (monitorManagerTest, scanObj2)
{
   monMonitorManager mgr ;

   // Turn on monitoring
   mgr.setMonitorStatus(MON_CLASS_EDU, TRUE) ;

   monClassReadScanner *scanner = mgr.getReadScanner(MON_CLASS_EDU, MON_CLASS_ACTIVE_LIST );
   monClassEDU *retObj = (monClassEDU*)scanner->getNext();

   ASSERT_EQ( retObj, (monClassEDU*)NULL ) ;

   delete ( scanner ) ;
}

// Test scanning empty archive list
TEST (monitorManagerTest, scanObj3)
{
   monMonitorManager mgr ;

   // Turn on monitoring
   mgr.setMonitorStatus(MON_CLASS_EDU, TRUE) ;

   monClassReadScanner *scanner = mgr.getReadScanner(MON_CLASS_EDU, MON_CLASS_ARCHIVED_LIST );
   monClassEDU *retObj = (monClassEDU*)scanner->getNext();

   ASSERT_EQ( retObj, (monClassEDU*)NULL ) ;
   delete ( scanner ) ;
}

// Test deleting objects. If archiving is off, then the object should be removed
TEST (monitorManagerTest, deleteObj1)
{
   monMonitorManager mgr ;
   // Turn on monitoring
   mgr.setMonitorStatus(MON_CLASS_EDU, TRUE) ;

   monClassEDU *obj = mgr.registerMonitorObject<monClassEDU>() ;

   monClassReadScanner *scanner = mgr.getReadScanner(MON_CLASS_EDU, MON_CLASS_ACTIVE_LIST );
   monClassEDU *retObj = (monClassEDU*)scanner->getNext();
   ASSERT_EQ(retObj, obj) ;

   delete ( scanner ) ;

   mgr.removeMonitorObject(obj) ;

   // Can no longer read this from a scan
   scanner = mgr.getReadScanner(MON_CLASS_EDU, MON_CLASS_ACTIVE_LIST ) ;
   retObj = (monClassEDU*)scanner->getNext() ;
   ASSERT_EQ( retObj, (monClassEDU*)NULL ) ;
   delete ( scanner ) ;

   // Object should be in pending delete state
   ASSERT_TRUE(obj->isPendingDelete()) ;
   ASSERT_FALSE(obj->isPendingArchive()) ;
}

// Test deleting objects. If archiving is on, can still see it when reading archive list
TEST (monitorManagerTest, deleteObj2)
{
   monMonitorManager mgr ;
   // Turn on monitoring
   mgr.setMonitorStatus(MON_CLASS_QUERY, TRUE) ;

   monClassQuery *obj = mgr.registerMonitorObject<monClassQuery>() ;

   monClassReadScanner *scanner = mgr.getReadScanner(MON_CLASS_QUERY, MON_CLASS_ACTIVE_LIST );
   monClassQuery *retObj = (monClassQuery*)scanner->getNext();
   ASSERT_EQ(retObj, obj) ;

   delete ( scanner ) ;
   mgr.removeMonitorObject(obj) ;

   // Can no longer read this from a active list scan
   scanner = mgr.getReadScanner(MON_CLASS_QUERY, MON_CLASS_ACTIVE_LIST ) ;
   retObj = (monClassQuery*)scanner->getNext() ;
   ASSERT_EQ( retObj, (monClassQuery*)NULL ) ;
   delete ( scanner ) ;
   // Object should be in pending archive state
   ASSERT_TRUE(obj->isPendingArchive()) ;

   // Can read this from an archive list scan
   scanner = mgr.getReadScanner(MON_CLASS_QUERY, MON_CLASS_ARCHIVED_LIST ) ;
   retObj = (monClassQuery*)scanner->getNext() ;
   ASSERT_EQ( retObj, obj ) ;
   delete ( scanner ) ;
}
*/


TEST (monitorLockTest, mutexLockRSUCCESS)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);

   ASSERT_EQ(mutex->getNumOwner(), (INT32)0);
   ASSERT_EQ(mutex->lastSOwnerTID, (INT32)0);
   ASSERT_EQ(mutex->xOwnerTID, (INT32)0);

   INT32 rc = mutex->lock_r();

   EXPECT_EQ(mutex->latchID, MON_LATCH_ID_MAX);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);
   EXPECT_EQ(rc, SDB_OK);
   //we shouldn't have any cb here
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   delete mutex;
}

TEST (monitorLockTest, mutexLockWSUCCESS)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);

   INT32 rc = mutex->lock_w();

   EXPECT_EQ(rc, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);
   delete mutex;
}

TEST (monitorLockTest, mutexLockRFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   INT32 rc1 = mutex->lock_w();
   INT32 rc2 = mutex->lock_r(100);

   EXPECT_EQ(rc1, SDB_OK);
   EXPECT_NE(rc2, SDB_OK);

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   delete mutex;
}

TEST (monitorLockTest, mutexLockWFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   INT32 rc1 = mutex->lock_r();
   INT32 rc2 = mutex->lock_w(100);

   EXPECT_EQ(rc1, SDB_OK);
   EXPECT_NE(rc2, SDB_OK);

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   delete mutex;
}


TEST (monitorLockTest, mutexTryLockRSUCCESS)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc = mutex->try_lock_r();

   EXPECT_TRUE(rc);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->getNumOwner(), (INT32)1);

   rc = mutex->try_lock_r();

   EXPECT_TRUE(rc);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->getNumOwner(), (INT32)2);

   delete mutex;
}

TEST (monitorLockTest, mutexTryLockWSUCCESS)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc = mutex->try_lock_w();

   EXPECT_TRUE(rc);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);
   delete mutex;
}


TEST (monitorLockTest, mutexTryLockRFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc1 = mutex->try_lock_w();
   BOOLEAN rc2 = mutex->try_lock_r();

   EXPECT_TRUE(rc1);
   EXPECT_FALSE(rc2);

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);

   delete mutex;
}

TEST (monitorLockTest, mutexTryLockWFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc1 = mutex->try_lock_r();
   BOOLEAN rc2 = mutex->try_lock_w();

   EXPECT_TRUE(rc1);
   EXPECT_FALSE(rc2);

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);
   EXPECT_EQ(mutex->lastSOwnerTID, (INT32)0);
   EXPECT_EQ(mutex->xOwnerTID, (INT32)0);

   delete mutex;
}

TEST (monitorLockTest, mutexRReleaseSuccess)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc1 = mutex->try_lock_r();
   EXPECT_TRUE(rc1);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);

   INT32 rc2 = mutex->release_r();
   EXPECT_EQ(rc2, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);

   INT32 rc3 = mutex->lock_r();
   EXPECT_EQ(rc3, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);

   INT32 rc4 = mutex->release_r();
   EXPECT_EQ(rc4, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);
   delete mutex;
}

TEST (monitorLockTest, mutexWReleaseSuccess)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   BOOLEAN rc1 = mutex->try_lock_w();
   EXPECT_TRUE(rc1);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);

   INT32 rc2 = mutex->release_w();
   EXPECT_EQ(rc2, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);

   INT32 rc3 = mutex->lock_w();
   EXPECT_EQ(rc3, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)1);

   INT32 rc4 = mutex->release_w();
   EXPECT_EQ(rc4, SDB_OK);
   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);
   delete mutex;
}

TEST (monitorLockTest, mutexRReleaseFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   INT32 rc = mutex->release_r();

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);
   EXPECT_NE(rc, SDB_OK);
}


TEST (monitorLockTest, mutexWReleaseFail)
{
   monRWMutex *mutex = new monRWMutex(MON_LATCH_ID_MAX);
   INT32 rc = mutex->release_w();

   EXPECT_EQ(mutex->getNumOwner(), (SINT32)0);
   EXPECT_NE(rc, SDB_OK);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
