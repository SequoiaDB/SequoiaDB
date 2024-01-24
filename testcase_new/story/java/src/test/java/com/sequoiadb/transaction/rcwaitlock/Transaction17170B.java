package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:seqDB-17170：插入并发，过程中读 索引扫描
 * @Author zhaoyu
 * @Date 2019-01-22
 * @Version 1.00
 */
@Test(groups = "rcwaitlock")
public class Transaction17170B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17170B";
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    DBCollection cl = null;
    private ArrayList< BSONObject > expList = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > actList = new ArrayList< BSONObject >();
    private DBCursor cursor = null;
    private String hint = "{\"\":\"a\"}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1.setSessionAttr( ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
        db2.setSessionAttr( ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
        db3.setSessionAttr( ( BSONObject ) JSON.parse( "{TransTimeout:5}" ) );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        db3.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        // 先关闭事务连接，再删除集合
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {

        // 开启3个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl3 = db3.getCollectionSpace( csName )
                .getCollection( clName );

        // 事务1插入记录R1
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{a:1,b:1}" );
        cl1.insert( insertR1 );

        // 事务2插入记录R2，记录内容与R1相同
        BSONObject insertR2 = ( BSONObject ) JSON.parse( "{a:1,b:1}" );
        cl2.insert( insertR2 );

        // 事务1索引读
        QueryThread indexScanThread1 = new QueryThread( cl1 );
        indexScanThread1.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                indexScanThread1.getTransactionID() ) );

        // 事务2索引读
        QueryThread indexScanThread2 = new QueryThread( cl2 );
        indexScanThread2.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                indexScanThread2.getTransactionID() ) );

        // 事务3索引读
        QueryThread indexScanThread3 = new QueryThread( cl3 );
        indexScanThread3.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                indexScanThread3.getTransactionID() ) );

        // 非事务索引读
        expList.add( insertR1 );
        expList.add( insertR2 );
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        try {
            // 校验索引等锁超时
            Assert.assertEquals( indexScanThread1.getExecResult(), -13 );
            Assert.assertEquals( indexScanThread2.getExecResult(), -13 );
            Assert.assertEquals( indexScanThread3.getExecResult(), -13 );

        } catch ( InterruptedException e ) {
            e.printStackTrace();
            Assert.fail();
        }

        // 事务3再次索引读
        TransUtils.beginTransaction( db3 );
        try {
            DBCursor indexCursor = cl3.query( null, null, null, hint );
            while ( indexCursor.hasNext() ) {
            }
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -13 );
        }

        // 提交事务1
        db1.commit();

        // 提交事务2
        db2.commit();

        // 非事务索引读
        cursor = cl.query( null, null, "{_id:1}", hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 提交事务3
        db3.commit();

        // 删除记录
        cl.delete( ( BSONObject ) null );

        // 非事务索引读
        expList.clear();
        cursor = cl.query( null, null, null, hint );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

    }

    private class QueryThread extends SdbThreadBase {
        private DBCollection cl = null;

        public QueryThread( DBCollection cl ) {
            super();
            this.cl = cl;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            try {
                List< BSONObject > ret = new ArrayList< BSONObject >();
                DBCursor indexCursor = cl.query( null, null, null, hint );
                while ( indexCursor.hasNext() ) {
                    ret.add( indexCursor.getNext() );
                }
                throw new BaseException( 1000, "NEED ERROR CODE -13" );
            } catch ( BaseException e ) {
                setExecResult( e.getErrorCode() );
            }
        }
    }
}
