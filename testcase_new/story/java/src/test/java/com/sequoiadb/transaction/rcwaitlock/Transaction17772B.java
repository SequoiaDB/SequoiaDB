package com.sequoiadb.transaction.rcwaitlock;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
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
 * @FileName:seqDB-17772：删除与更新并发，删除的记录同时匹配已提交记录及其他事务更新的记录，事务回滚，过程中读 索引扫描, R1>R2
 * @Author zhaoyu
 * @Date 2019-01-29
 * @Version 1.00
 */
@Test(groups = "rcwaitlock")
public class Transaction17772B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_17772B";
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private Sequoiadb db4 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR1 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private ArrayList< BSONObject > actList = new ArrayList< >();
    private DBCursor cursor = null;
    private String hint = "{\"\":\"a\"}";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17772B_1',a:2,b:2}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17772B_2',a:1,b:1}" );
        updateR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17772B_1',a:4,b:4}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17772B_2',a:3,b:3}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        // 正序索引，正序索引读
        List< BSONObject > expPositiveReadList1 = new ArrayList< >();
        expPositiveReadList1.add( updateR2 );

        // 事务提交，正序索引，正序索引读
        List< BSONObject > expPositiveReadList2 = new ArrayList< >();
        expPositiveReadList2.add( updateR2 );
        expPositiveReadList2.add( updateR1 );

        // 事务提交，正序索引，逆序索引读
        List< BSONObject > expReverseReadList2 = new ArrayList< >();
        expReverseReadList2.add( updateR1 );
        expReverseReadList2.add( updateR2 );

        // 逆序索引，正序索引读
        List< BSONObject > expPositiveReadList3 = new ArrayList< >();
        expPositiveReadList3.add( insertR2 );

        // 逆序索引，正序索引读
        List< BSONObject > expPositiveReadList4 = new ArrayList< >();
        expPositiveReadList4.add( insertR2 );
        expPositiveReadList4.add( updateR1 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expPositiveReadList1,
                        expPositiveReadList2, expReverseReadList2,
                        expPositiveReadList2, expPositiveReadList1 },
                { "{'a': -1}", expPositiveReadList3, expPositiveReadList3,
                        expPositiveReadList2, expReverseReadList2,
                        expPositiveReadList4, new ArrayList< BSONObject >() } };
    }

    @SuppressWarnings("unchecked")
    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expPositiveReadList1,
            List< BSONObject > expReverseReadList1,
            List< BSONObject > expPositiveReadList2,
            List< BSONObject > expReverseReadList2,
            List< BSONObject > expPositiveReadList3,
            List< BSONObject > expReverseReadList3 )
            throws InterruptedException {
        try {

            // 开启4个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            TransUtils.beginTransaction( db4 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
            cl4 = db4.getCollectionSpace( csName ).getCollection( clName );

            // 插入记录R1、R2，R1<R2
            cl.createIndex( "a", indexKey, false, false );
            cl.insert( insertR1 );
            cl.insert( insertR2 );

            // 事务1匹配R1删除
            cl1.delete( "{a:2}", hint );

            // 事务2匹配R1、R2更新为R3、R4,R1<R2<R3
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务3记录读
            TransactionQueryThread tableScanThread1 = new TransactionQueryThread(
                    cl3, "{a:1}" );
            tableScanThread1.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread1.getTransactionID() ) );

            // 事务3记录读
            TransactionQueryThread tableScanThread2 = new TransactionQueryThread(
                    cl4, "{a:-1}" );
            tableScanThread2.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread2.getTransactionID() ) );

            // 非事务读,正序
            cursor = cl.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList1 );
            actList.clear();

            // 非事务读,逆序
            cursor = cl.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList1 );
            actList.clear();

            // 回滚事务1
            db1.rollback();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread1.getTransactionID() ) );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    tableScanThread2.getTransactionID() ) );

            // 非事务记录读，正序
            cursor = cl.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList2 );
            actList.clear();

            // 事务2记录读，正序
            cursor = cl2.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList2 );
            actList.clear();

            // 非事务记录读，逆序
            cursor = cl.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReverseReadList2 );
            actList.clear();

            // 事务2记录读，逆序
            cursor = cl2.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReverseReadList2 );
            actList.clear();

            // 提交事务2
            db2.commit();
            Assert.assertTrue( tableScanThread1.isSuccess(),
                    tableScanThread1.getErrorMsg() );
            Assert.assertTrue( tableScanThread2.isSuccess(),
                    tableScanThread2.getErrorMsg() );

            try {
                // 校验事务3读返回的记录
                actList = ( ArrayList< BSONObject > ) tableScanThread1
                        .getExecResult();
                Assert.assertEquals( actList, expPositiveReadList3 );
                actList.clear();

                // 校验事务4读返回的记录
                actList = ( ArrayList< BSONObject > ) tableScanThread2
                        .getExecResult();
                Assert.assertEquals( actList, expReverseReadList3 );
                actList.clear();

            } catch ( InterruptedException e ) {
                e.printStackTrace();
                Assert.fail();
            }

            // 非事务记录读，正序
            cursor = cl.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList2 );
            actList.clear();

            // 事务3读记录，正序
            cursor = cl3.query( null, null, "{a:1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expPositiveReadList2 );
            actList.clear();

            // 非事务记录读，逆序
            cursor = cl.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReverseReadList2 );
            actList.clear();

            // 事务3读记录，逆序
            cursor = cl3.query( null, null, "{a:-1}", hint );
            actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expReverseReadList2 );
            actList.clear();

            // 提交事务3
            db3.commit();
            db4.commit();
        } finally {
            // 关闭事务连接
            db1.commit();
            db2.commit();
            db3.commit();
            db4.commit();

            // 删除索引
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }

            // 删除记录
            cl.truncate();

        }
    }

    @AfterClass
    public void tearDown() {
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        if ( !db4.isClosed() ) {
            db4.close();
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

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( null, "{$inc:{a:2,b:2}}", hint );
        }
    }

    private class TransactionQueryThread extends SdbThreadBase {
        private DBCollection cl = null;
        private String sort = null;

        public TransactionQueryThread( DBCollection cl, String sort ) {
            super();
            this.cl = cl;
            this.sort = sort;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            DBCursor indexCursor = cl.query( null, null, sort, hint );
            List< BSONObject > actQueryList = TransUtils
                    .getReadActList( indexCursor );
            setExecResult( actQueryList );
            indexCursor.close();
        }
    }
}
