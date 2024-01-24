package com.sequoiadb.transaction.lockEscalation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @Description: seqDB-24853:验证锁升级的事务日志量限制
 * @Author Yang Qincheng
 * @Date 2021.12.15
 */
@Test( groups = "lockEscalation" )
public class Transaction24853 extends SdbTestBase {
    private Sequoiadb db;
    private DBCollection cl1;
    private DBCollection cl2;
    private final String clName1 = "cl_24853A";
    private final String clName2 = "cl_24853B";
    private String nodeName1 = "";
    private String nodeName2 = "";
    private double maxLogSpace = 0;
    private boolean isStandAlone = false;
    private final int transMaxLogSpaceRatio = 1;
    private final static int insertNum = 10000;
    private final static String LOGFILE_NUM = "logfilenum";
    private final static String LOGFILE_SIZE = "logfilesz";
    private final static String USED_LOGSPACE = "UsedLogSpace";
    private final static String NODE_NAME = "NodeName";

    @BeforeClass()
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        isStandAlone = CommLib.isStandAlone( db );
        BSONObject option1 = new BasicBSONObject();
        BSONObject option2 = new BasicBSONObject();
        if ( !isStandAlone ) {
            ArrayList< String > groupList = CommLib.getDataGroupNames( db );
            if ( groupList.size() < 2 ) {
                throw new SkipException( "less two groups skip testcase" );
            }
            int num = 0;
            option1.put( "Group", groupList.get( num ) );
            nodeName1 = db.getReplicaGroup( groupList.get( num ) ).getMaster().getNodeName();
            num = ++ num < groupList.size() - 1 ? num : groupList.size() - 1;
            option2.put( "Group", groupList.get( num ) );
            nodeName2 = db.getReplicaGroup( groupList.get( num ) ).getMaster().getNodeName();
        }
        cl1 = db.getCollectionSpace( csName ).createCollection( clName1, option1 );
        cl2 = db.getCollectionSpace( csName ).createCollection( clName2, option2 );

        BSONObject sessionAttr = new BasicBSONObject();
        sessionAttr.put( "TransMaxLockNum", -1 );
        sessionAttr.put( "TransMaxLogSpaceRatio", transMaxLogSpaceRatio );
        db.setSessionAttr( sessionAttr );
        maxLogSpace = getMaxLogSpace();
    }

    @AfterClass()
    public void tearDown() {
        try {
            db.getCollectionSpace( csName ).dropCollection( clName1 );
            db.getCollectionSpace( csName ).dropCollection( clName2 );
        } finally {
            db.close();
        }
    }

    @Test
    public void test() {
        db.beginTransaction();
        try {
            LockEscalationUtil.insertData( db, csName, clName1, insertNum );
            LockEscalationUtil.insertData( db, csName, clName2, insertNum );
            long usedLogSpace1 = getUsedLogSpace( nodeName1 );
            long usedLogSpace2 = getUsedLogSpace( nodeName2 );
            if ( usedLogSpace1 + usedLogSpace2 >= maxLogSpace ) {
                Assert.fail( "The amount of data inserted at one time is too large" );
            }

            while ( usedLogSpace1 + usedLogSpace2 <= maxLogSpace ) {
                LockEscalationUtil.insertData( db, csName, clName1, insertNum );
                LockEscalationUtil.insertData( db, csName, clName2, insertNum );
                usedLogSpace1 = getUsedLogSpace( nodeName1 );
                usedLogSpace2 = getUsedLogSpace( nodeName2 );
            }

            try {
                while ( usedLogSpace1 <= maxLogSpace ) {
                    LockEscalationUtil.insertData( db, csName, clName1, insertNum );
                    usedLogSpace1 = getUsedLogSpace( nodeName1 );
                }
                Assert.fail( "The amount of logs used by transaction " + TransUtils.getTransactionID( db )
                        + " exceeded the limit!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOG_SPACE_UP_TO_LIMIT.getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            db.commit();
        }

        try ( DBCursor cursor1 = cl1.query();
              DBCursor cursor2 = cl2.query() ) {
            Assert.assertFalse( cursor1.hasNext() );
            Assert.assertFalse( cursor2.hasNext() );
        }
    }

    private double getMaxLogSpace() {
        double result = 0;
        // The logFileNum and logFileSize of each date node in the same cluster are the same
        BSONObject matcher = new BasicBSONObject();
        if ( !isStandAlone ) {
            matcher.put( NODE_NAME, nodeName1 );
        }
        BSONObject selector = new BasicBSONObject();
        selector.put( LOGFILE_NUM, "" );
        selector.put( LOGFILE_SIZE, "" );
        try ( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, matcher, selector, null ) ) {
            BSONObject obj = cursor.getNext();
            int logFileNum = ( int ) obj.get( LOGFILE_NUM );
            int logFileSize = ( int ) obj.get( LOGFILE_SIZE );  // unit is MB
            result = ( logFileNum * logFileSize * 1024L * 1024L ) * ( transMaxLogSpaceRatio * 0.01 );
        }
        return result;
    }

    private long getUsedLogSpace( String nodeName ) {
        long result = 0;
        BSONObject matcher = new BasicBSONObject();
        if ( !isStandAlone ) {
            matcher.put( NODE_NAME, nodeName );
        }
        BSONObject selector = new BasicBSONObject();
        selector.put( USED_LOGSPACE, "" );
        try ( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT, matcher, selector, null ) ) {
            BSONObject obj = cursor.getNext();
            result = ( long ) obj.get( USED_LOGSPACE );
        }
        return result;
    }
}