package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description: Lock escalation utility class
 * @Author Yang Qincheng
 * @Date 2021.12.10
 */
public class LockEscalationUtil {

    public static final String LOCK_IS = "IS";
    public static final String LOCK_IX = "IX";
    public static final String LOCK_S = "S";
    public static final String LOCK_SIX = "SIX";
    public static final String LOCK_U = "U";
    public static final String LOCK_X = "X";
    public static final String LOCK_Z = "Z";

    public static final BSONObject EMPTY_BSON = new BasicBSONObject();
    public static final int THREAD_TIMEOUT = 600000; // timeout 10 mins

    public static void insertData( Sequoiadb db, String csName, String clName, int recordNum ) {
        DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
        List< BSONObject > docList = new ArrayList<>();
        for ( int i = 0; i < recordNum; i++ ) {
            docList.add( new BasicBSONObject( "a", i ) );
        }
        cl.insert( docList );
    }

    public static void queryData( Sequoiadb db, String csName, String clName, long skipRowsCount, long returnRowsCount ) {
        DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
        DBQuery matcher = new DBQuery();
        matcher.setSkipRowsCount( skipRowsCount );
        matcher.setReturnRowsCount( returnRowsCount );
        try ( DBCursor cursor = cl.query( matcher ) ) {
            while ( cursor.hasNext() ) {
                cursor.getNext();
            }
        }
    }

    public static void checkCLLockType( Sequoiadb db, String lockMode ) {
        try ( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, "", "", "" ) ) {
            Assert.assertTrue( cursor.hasNext() );
            BSONObject record = cursor.getNext();
            BasicBSONList lockList = ( BasicBSONList ) record.get( "GotLocks" );
            Assert.assertNotNull( lockList );
            for ( Object obj : lockList ) {
                LockBean lock = new LockBean( ( BSONObject ) obj );
                if ( lock.isCLLock() ) {
                    Assert.assertEquals( lock.getMod(), lockMode, lock.toString() );
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
            if ( csID >= 0 && clID >= 0 && clID != 65535 && extentID == - 1 && offset == - 1 ) {
                result = true;
            }
            return result;
        }

        public String getMod() {
            return mod;
        }

        @Override
        public String toString() {
            return "LockBean{" +
                    "csID=" + csID +
                    ", clID=" + clID +
                    ", extentID=" + extentID +
                    ", offset=" + offset +
                    ", mod='" + mod + '\'' +
                    ", count=" + count +
                    ", duration=" + duration +
                    '}';
        }
    }
}