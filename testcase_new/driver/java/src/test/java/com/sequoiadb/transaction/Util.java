package com.sequoiadb.transaction;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import java.util.List;

public class Util {
    public static final String LOCK_IS = "IS";
    public static final String LOCK_IX = "IX";
    public static final String LOCK_S = "S";
    public static final String LOCK_SIX = "SIX";
    public static final String LOCK_U = "U";
    public static final String LOCK_X = "X";
    public static final String LOCK_Z = "Z";

    public static boolean isCluster( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            int errno = e.getErrorCode();
            if ( new BaseException( "SDB_RTN_COORD_ONLY" )
                    .getErrorCode() == errno ) {
                System.out.println(
                        "This test is for cluster environment only." );
                return false;
            }
        }
        return true;
    }

    public static void checkIsLockEscalated( Sequoiadb db,
            Boolean isLockEscalated ) {
        try ( DBCursor cursor = db.getSnapshot(
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "" )) {
            Assert.assertTrue( cursor.hasNext() );
            BSONObject record = cursor.getNext();
            Boolean actLockEscalated = ( Boolean ) record
                    .get( "IsLockEscalated" );
            Assert.assertEquals( actLockEscalated, isLockEscalated );
        }
    }

    public static void checkRecords( List< BSONObject > expRecord,
            DBCursor cursor ) {
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            Assert.assertEquals( record, expRecord.get( count++ ) );
        }
        cursor.close();
        Assert.assertEquals( count, expRecord.size() );
    }

    public static int getCLLockCount( Sequoiadb db, String lockType ) {
        try ( DBCursor cursor = db.getSnapshot(
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "" )) {
            int lockCount = 0;
            Assert.assertTrue( cursor.hasNext() );
            BSONObject record = cursor.getNext();
            BasicBSONList lockList = ( BasicBSONList ) record.get( "GotLocks" );
            for ( Object obj : lockList ) {
                int csID = ( int ) ( ( BSONObject ) obj ).get( "CSID" );
                int clID = ( int ) ( ( BSONObject ) obj ).get( "CLID" );
                int extentID = ( int ) ( ( BSONObject ) obj ).get( "ExtentID" );
                int offset = ( int ) ( ( BSONObject ) obj ).get( "Offset" );
                String mode = ( String ) ( ( BSONObject ) obj ).get( "Mode" );
                if ( csID >= 0 && clID >= 0 && extentID >= 0 && offset >= 0 ) {
                    if ( lockType.equals( mode ) ) {
                        lockCount++;
                    }
                }
            }
            return lockCount;
        }
    }

    public static void checkCLLockType( Sequoiadb db, String lockMode ) {
        try ( DBCursor cursor = db.getSnapshot(
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "" )) {
            Assert.assertTrue( cursor.hasNext() );
            BSONObject record = cursor.getNext();
            BasicBSONList lockList = ( BasicBSONList ) record.get( "GotLocks" );
            Assert.assertNotNull( lockList );
            for ( Object obj : lockList ) {
                LockBean lock = new LockBean( ( BSONObject ) obj );
                if ( lock.isCLLock() ) {
                    Assert.assertEquals( lock.getMod(), lockMode,
                            lock.toString() );
                }
            }
        }
    }

    /**
     * @Description: Lock object in transaction snapshot results
     */
    static class LockBean {
        private final int csID;
        private final int clID;
        private final int extentID;
        private final int offset;
        private final String mod;
        private final int count;
        private final long duration;

        public LockBean( BSONObject obj ) {
            this.csID = ( int ) obj.get( "CSID" );
            this.clID = ( int ) obj.get( "CLID" );
            this.extentID = ( int ) obj.get( "ExtentID" );
            this.offset = ( int ) obj.get( "Offset" );
            this.mod = ( String ) obj.get( "Mode" );
            this.count = ( int ) obj.get( "Count" );
            this.duration = ( long ) obj.get( "Duration" );
        }

        public boolean isCLLock() {
            boolean result = false;
            if ( csID >= 0 && clID >= 0 && clID != 65535 && extentID == -1
                    && offset == -1 ) {
                result = true;
            }
            return result;
        }

        public String getMod() {
            return mod;
        }

        @Override
        public String toString() {
            return "LockBean{" + "csID=" + csID + ", clID=" + clID
                    + ", extentID=" + extentID + ", offset=" + offset
                    + ", mod='" + mod + '\'' + ", count=" + count
                    + ", duration=" + duration + '}';
        }
    }

}
