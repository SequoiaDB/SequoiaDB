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
 * @Description seqDB-17761.java
 *              插入与更新并发，更新的记录同时匹配已提交记录及其他事务插入的记录，更新走索引，事务提交，过程中读 R2<R1<R3
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rcwaitlock")
public class Transaction17761D extends SdbTestBase {

    private String clName = "transCL_17761D";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private Sequoiadb sdb4 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR1 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private DBCursor recordCur = null;
    private List< BSONObject > actDataList = new ArrayList< >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17761D_1',a:2,b:2}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17761D_2',a:1,b:1}" );
        updateR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17761D_1',a:4,b:4}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17761D_2',a:3,b:3}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        // 正序索引，正序索引读
        List< BSONObject > expPositiveReadList1 = new ArrayList< >();
        expPositiveReadList1.add( insertR2 );
        expPositiveReadList1.add( insertR1 );

        // 正序索引，逆序索引读
        List< BSONObject > expReverseReadList1 = new ArrayList< >();
        expReverseReadList1.add( insertR1 );
        expReverseReadList1.add( insertR2 );

        // 事务提交，正序索引，正序索引读
        List< BSONObject > expPositiveReadList2 = new ArrayList< >();
        expPositiveReadList2.add( updateR2 );
        expPositiveReadList2.add( updateR1 );

        // 事务提交，正序索引，逆序索引读
        List< BSONObject > expReverseReadList2 = new ArrayList< >();
        expReverseReadList2.add( updateR1 );
        expReverseReadList2.add( updateR2 );

        // 事务提交，正序索引，事务中逆序索引读
        List< BSONObject > expReverseReadList3 = new ArrayList< >();
        expReverseReadList3.add( insertR1 );
        expReverseReadList3.add( updateR2 );

        // 事务提交，正序索引，事务中正序索引读
        List< BSONObject > expPositiveReadList3 = new ArrayList< >();
        expPositiveReadList3.add( insertR1 );

        // 逆序索引，正序索引读
        List< BSONObject > expPositiveReadList4 = new ArrayList< >();
        expPositiveReadList4.add( insertR2 );
        expPositiveReadList4.add( updateR1 );

        // 逆序索引，逆序索引读
        List< BSONObject > expReverseReadList4 = new ArrayList< >();
        expReverseReadList4.add( updateR1 );
        expReverseReadList4.add( insertR2 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2,
                        expPositiveReadList2, expPositiveReadList3 },
                { "{'a': -1}", expPositiveReadList4, expReverseReadList4,
                        expPositiveReadList2, expReverseReadList2,
                        expPositiveReadList2, expReverseReadList2 } };
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
            cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );
            cl4 = sdb4.getCollectionSpace( csName ).getCollection( clName );

            // 开启事务
            TransUtils.beginTransaction( sdb1 );
            TransUtils.beginTransaction( sdb2 );
            TransUtils.beginTransaction( sdb3 );
            TransUtils.beginTransaction( sdb4 );

            // 插入记录
            cl.createIndex( "a", indexKey, false, false );
            cl.insert( insertR1 );

            // 事务1插入记录R2
            cl1.insert( insertR2 );

            // 事务2更新记录R1及R2
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务3索引读,正序
            QueryThread queryThread1 = new QueryThread( cl3, "{a:1}" );
            queryThread1.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread1.getTransactionID() ) );

            // 事务4索引读,逆序
            QueryThread queryThread2 = new QueryThread( cl4, "{a:-1}" );
            queryThread2.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread2.getTransactionID() ) );

            // 非事务记录读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

            // 非事务索引读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

            // 非事务记录读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
            actDataList.clear();

            // 非事务索引读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
            actDataList.clear();

            // 提交事务1
            sdb1.commit();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread1.getTransactionID() ) );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread2.getTransactionID() ) );

            // 非事务记录读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 非事务索引读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 事务2记录读，正序
            recordCur = cl2.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 事务2索引读，正序
            recordCur = cl2.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 非事务记录读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 非事务索引读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 事务2记录读，逆序
            recordCur = cl2.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 事务2索引读，逆序
            recordCur = cl2.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 提交事务2
            sdb2.commit();
            Assert.assertTrue( queryThread1.isSuccess(),
                    queryThread1.getErrorMsg() );
            Assert.assertTrue( queryThread2.isSuccess(),
                    queryThread2.getErrorMsg() );

            try {
                // 校验事务3读返回的记录（正序）
                actDataList = ( ArrayList< BSONObject > ) queryThread1
                        .getExecResult();
                Assert.assertEquals( actDataList, expPositiveReadList3 );
                actDataList.clear();

                // 校验事务4读返回的记录(逆序),阻塞在updateR1上，事务2提交后，updateR2<updateR1,索引无法往回读，因此只读到updateR1
                actDataList = ( ArrayList< BSONObject > ) queryThread2
                        .getExecResult();
                Assert.assertEquals( actDataList, expReverseReadList3 );
                actDataList.clear();

            } catch ( InterruptedException e ) {
                e.printStackTrace();
                Assert.fail();
            }

            // 非事务记录读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 非事务索引读，正序
            recordCur = cl.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 事务3记录读，正序
            recordCur = cl3.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 事务3索引读，正序
            recordCur = cl3.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList2 );
            actDataList.clear();

            // 非事务记录读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 非事务索引读，逆序
            recordCur = cl.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 事务3记录读，逆序
            recordCur = cl3.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 事务3索引读，逆序
            recordCur = cl3.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList2 );
            actDataList.clear();

            // 提交事务3、事务4
            sdb3.commit();
            sdb4.commit();
        } finally {
            // 关闭事务连接
            sdb1.commit();
            sdb2.commit();
            sdb3.commit();
            sdb4.commit();

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
        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( sdb3 != null ) {
            sdb3.close();
        }
        if ( sdb4 != null ) {
            sdb4.close();
        }

        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.update( null, "{'$inc': {'a': 2, 'b': 2}}", "{'': 'a'}" );
        }
    }

    private class QueryThread extends SdbThreadBase {
        DBCollection cl = null;
        String sort = null;

        public QueryThread( DBCollection cl, String sort ) {
            super();
            this.cl = cl;
            this.sort = sort;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            DBCursor cur = cl.query( null, null, sort, "{'': 'a'}" );
            List< BSONObject > actQueryList = TransUtils.getReadActList( cur );
            setExecResult( actQueryList );
            cur.close();
        }
    }

}
