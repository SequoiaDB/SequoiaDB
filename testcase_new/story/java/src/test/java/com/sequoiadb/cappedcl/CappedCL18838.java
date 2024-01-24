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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName: seqDB-18838:插入/pop/query/truncate时，删除固定集合
 * 
 * @author zhaoyu
 * @Date 2019.7.22
 */
public class CappedCL18838 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cappedCS = null;
    private DBCollection cappedCL = null;
    private String cappedCLName = "cappedCL_18838";
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 100 );
    private int insertNum = 10000;
    private ThreadExecutor te = new ThreadExecutor( 1800000 );
    private int threadNum = 3;
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cappedCS = sdb.getCollectionSpace( cappedCSName );
        cappedCL = cappedCS.createCollection( cappedCLName,
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
            te.addWorker( new InsertThread() );
            te.addWorker( new PopThread() );
            te.addWorker( new QueryThread() );
            te.addWorker( new TruncateThread() );
        }
        DropCLThread dropCLThread = new DropCLThread();
        te.addWorker( dropCLThread );

        te.run();

        // 插入与pop并发不校验_id值，校验主备一致性
        if ( dropCLThread.getRetCode() != 0 ) {
            Assert.assertTrue(
                    CappedCLUtils.isLSNConsistency( sdb, groupName ) );
            Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb,
                    cappedCSName, cappedCLName ) );
        } else {
            Assert.assertFalse(
                    ( cappedCS.isCollectionExist( cappedCLName ) ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cappedCS.isCollectionExist( cappedCLName ) ) {
                cappedCS.dropCollection( cappedCLName );
            }

        } finally {
            sdb.close();
        }
    }

    private class InsertThread {
        @ExecuteOrder(step = 1, desc = "插入记录")
        public void insert() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                for ( int i = 0; i < 10000; i++ ) {
                    BasicBSONObject insertObj = new BasicBSONObject();
                    insertObj.put( "a", strBuffer.toString() );
                    cl.insert( insertObj );
                }
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -23 ) {
                    throw e;
                }
            } finally {
                db.close();
            }

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
                if ( lids.size() != 0 ) {
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
                if ( e.getErrorCode() != -6 && e.getErrorCode() != -321
                        && e.getErrorCode() != -23 ) {
                    throw e;
                }
            } finally {
                db.close();
            }

        }
    }

    private class QueryThread {

        @ExecuteOrder(step = 1, desc = "查询记录")
        public void insert() {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                DBCursor cursor = cl.query();
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                }
                cursor.close();
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -23 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class TruncateThread {
        @ExecuteOrder(step = 1, desc = "truncate记录")
        public void insert() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                cl.truncate();
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -23
                        && e.getErrorCode() != -190 ) {
                    throw e;
                }
            } finally {
                db.close();
            }

        }
    }

    private class DropCLThread extends ResultStore {
        @ExecuteOrder(step = 1, desc = "删除集合")
        public void dropcappedCL() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( cappedCSName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                cs.dropCollection( cappedCLName );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 && e.getErrorCode() != -190 ) {
                    throw e;
                }
                saveResult( e.getErrorCode(), e );
            } finally {
                db.close();
            }

        }
    }
}
