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
 * FileName: seqDB-18837：单个固定集合空间下，并发创建多个固定集合与数据操作并发
 * 
 * @author zhaoyu
 * @Date 2019.7.22
 */
public class CappedCL18837 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String cappedCLName = "cappedCL_18837";
    private CollectionSpace cappedCS = null;
    private int clNum = 3;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 2000 );
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
                ( BSONObject ) JSON.parse(
                        "{Capped:true,Size:1024,Group:'" + groupName + "'}" ) );

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
        BasicBSONObject insertObj = new BasicBSONObject();
        insertObj.put( "a", strBuffer.toString() );
        CappedCLUtils.insertRecords( cappedCL, insertObj, 10000 );
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < clNum; i++ ) {
            te.addWorker( new CreateCLThread( cappedCLName + "_" + i ) );
        }
        te.addWorker( new InsertThread() );
        te.addWorker( new PopThread() );
        te.addWorker( new QueryThread() );
        te.addWorker( new TruncateThread() );
        te.run();

        // 校验主备一致性
        Assert.assertTrue( CappedCLUtils.isLSNConsistency( sdb, groupName ) );
        Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb, cappedCSName,
                cappedCLName ) );

    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < clNum; i++ ) {
                cappedCS.dropCollection( cappedCLName + "_" + i );
            }
            cappedCS.dropCollection( cappedCLName );
        } finally {
            sdb.close();
        }
    }

    private class CreateCLThread {
        String clName = null;

        public CreateCLThread( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "创建集合")
        public void createCappedCL() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                db.getCollectionSpace( cappedCSName ).createCollection( clName,
                        ( BSONObject ) JSON
                                .parse( "{Capped:true, Size:1024, Group:'"
                                        + groupName + "'}" ) );
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } finally

            {
                db.close();
            }

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
                if ( e.getErrorCode() != -321 ) {
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
                if ( e.getErrorCode() != -6 && e.getErrorCode() != -321 ) {
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
                if ( e.getErrorCode() != -321 ) {
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
                if ( e.getErrorCode() != -321 ) {
                    throw e;
                }
            } finally {
                db.close();
            }

        }
    }
}
