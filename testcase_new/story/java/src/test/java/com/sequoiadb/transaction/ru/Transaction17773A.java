package com.sequoiadb.transaction.ru;

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
 * @Description seqDB-17773.java
 *              插入与更新并发，更新的记录同时匹配已提交记录及其他事务插入的记录，更新走索引，事务提交，过程中读 R1<R2
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "ru")
public class Transaction17773A extends SdbTestBase {

    private String clName = "transCL_17773A";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCursor recordCur = null;
    private BSONObject insertR1 = new BasicBSONObject();
    private BSONObject insertR2 = new BasicBSONObject();
    private BSONObject updateR1 = new BasicBSONObject();
    private BSONObject updateR2 = new BasicBSONObject();
    private List< BSONObject > expDataList = new ArrayList< BSONObject >();
    private List< BSONObject > actDataList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        insertR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17773A_1',a:1,b:1}" );
        insertR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17773A_2',a:2,b:2}" );
        updateR1 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17773A_1',a:3,b:3}" );
        updateR2 = ( BSONObject ) JSON
                .parse( "{_id:'insertID17773A_2',a:4,b:4}" );
    }

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        // 正序索引，正序索引读
        List< BSONObject > expPositiveReadList1 = new ArrayList< BSONObject >();
        expPositiveReadList1.add( insertR2 );
        expPositiveReadList1.add( updateR1 );

        // 正序索引，逆序索引读
        List< BSONObject > expReverseReadList1 = new ArrayList< BSONObject >();
        expReverseReadList1.add( updateR1 );
        expReverseReadList1.add( insertR2 );

        // 事务提交，正序索引，正序索引读
        List< BSONObject > expPositiveReadList2 = new ArrayList< BSONObject >();
        expPositiveReadList2.add( updateR1 );
        expPositiveReadList2.add( updateR2 );

        // 事务提交，正序索引，逆序索引读
        List< BSONObject > expReverseReadList2 = new ArrayList< BSONObject >();
        expReverseReadList2.add( updateR2 );
        expReverseReadList2.add( updateR1 );

        // 逆序索引，正序索引读
        List< BSONObject > expPositiveReadList3 = new ArrayList< BSONObject >();
        expPositiveReadList3.add( insertR1 );
        expPositiveReadList3.add( insertR2 );

        // 逆序索引，逆序索引读
        List< BSONObject > expReverseReadList3 = new ArrayList< BSONObject >();
        expReverseReadList3.add( insertR2 );
        expReverseReadList3.add( insertR1 );

        return new Object[][] {
                { "{'a': 1}", expPositiveReadList1, expReverseReadList1,
                        expPositiveReadList2, expReverseReadList2 },
                { "{'a': -1}", expPositiveReadList3, expReverseReadList3,
                        expPositiveReadList2, expReverseReadList2 } };
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, List< BSONObject > expPositiveReadList1,
            List< BSONObject > expReverseReadList1,
            List< BSONObject > expPositiveReadList2,
            List< BSONObject > expReverseReadList2 )
            throws InterruptedException {
        try {
            cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

            // 开启事务
            TransUtils.beginTransaction( sdb1 );
            TransUtils.beginTransaction( sdb2 );
            TransUtils.beginTransaction( sdb3 );

            // 插入记录R1
            cl.createIndex( "a", indexKey, false, false );
            cl.insert( insertR1 );

            // 事务1插入记录R2
            cl1.insert( insertR2 );

            // 事务2更新记录R1为R3，R2为R4
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 事务1记录读，正序
            recordCur = cl1.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

            // 事务1索引读，正序
            recordCur = cl1.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

            // 事务3记录读，正序
            recordCur = cl3.query( null, null, "{a:1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

            // 事务3索引读，正序
            recordCur = cl3.query( null, null, "{a:1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expPositiveReadList1 );
            actDataList.clear();

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

            // 事务1记录读，逆序
            recordCur = cl1.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
            actDataList.clear();

            // 事务1索引读，逆序
            recordCur = cl1.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
            actDataList.clear();

            // 事务3记录读，逆序
            recordCur = cl3.query( null, null, "{a:-1}", "{'': null}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
            actDataList.clear();

            // 事务3索引读，逆序
            recordCur = cl3.query( null, null, "{a:-1}", "{'': 'a'}" );
            actDataList = TransUtils.getReadActList( recordCur );
            Assert.assertEquals( actDataList, expReverseReadList1 );
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

            // 非事务记录读，正序
            expDataList.clear();
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
            expDataList.clear();
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

            // 提交事务2
            sdb2.commit();

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

            // 提交事务2
            sdb2.commit();

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

            // 提交事务3
            sdb3.commit();

        } finally {
            // 关闭事务连接
            sdb1.commit();
            sdb2.commit();
            sdb3.commit();

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
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( sdb != null ) {
            sdb.close();
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

}
