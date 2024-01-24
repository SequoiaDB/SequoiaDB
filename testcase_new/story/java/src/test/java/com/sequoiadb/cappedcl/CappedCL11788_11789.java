package com.sequoiadb.cappedcl;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName: seqDB-11788:pop并发指定LogicalID时，记录无交叉
 * seqDB-11789:pop并发指定LogicalID时，记录有交叉
 * 
 * @author zhaoyu
 * @Date 2019.7.17
 */
public class CappedCL11788_11789 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cappedCS = null;
    private String cappedCLName = "cappedCL_11788";
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 100 );
    private int threadNum = 5;
    private int insertNum = 10000;
    private ThreadExecutor te = new ThreadExecutor( 1800000 );
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cappedCS = sdb.getCollectionSpace( cappedCSName );
        DBCollection cappedCL = cappedCS.createCollection( cappedCLName,
                ( BSONObject ) JSON.parse( "{Capped:true, Size:10240,Group:'"
                        + groupName + "'}" ) );

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
        BasicBSONObject insertObj = new BasicBSONObject();
        insertObj.put( "a", strBuffer.toString() );
        CappedCLUtils.insertRecords( cappedCL, insertObj, insertNum );

    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < threadNum; i++ ) {
            te.addWorker( new PopThread() );
        }
        te.run();

        // 校验主备一致性
        Assert.assertTrue( CappedCLUtils.isLSNConsistency( sdb, groupName ) );
        Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb, cappedCSName,
                cappedCLName ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            cappedCS.dropCollection( cappedCLName );
        } finally {
            sdb.close();
        }
    }

    private class PopThread {
        @ExecuteOrder(step = 1, desc = "pop记录")
        public void pop() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );

                ArrayList< Long > lids = new ArrayList<>();

                // 获取_id值
                DBCursor cursor = cl.query( null, null, "{_id:1}", null );
                while ( cursor.hasNext() ) {
                    long _id = ( long ) cursor.getNext().get( "_id" );
                    lids.add( _id );
                }
                cursor.close();

                // 获取pop的logicalID
                int pos = 0;
                long logicalID = 0;
                if ( lids.size() > 0 ) {
                    pos = new Random().nextInt( lids.size() );
                    logicalID = lids.get( pos );
                }
                System.out.println( "random logicalID: " + logicalID );
                // pop记录
                BSONObject popObj = new BasicBSONObject();
                popObj.put( "LogicalID", logicalID );
                popObj.put( "Direction", ( pos % 2 == 0 ) ? -1 : 1 );

                // 并发pop时，logicalID对应的记录可能被其他pop线程pop了，该记录可能不存在，需要规避-6的错误码
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                cl.pop( popObj );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -6 ) {
                    throw e;
                }
            } finally {
                db.close();
            }

        }
    }
}
