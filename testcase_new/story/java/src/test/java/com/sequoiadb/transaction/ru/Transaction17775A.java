package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-17775.java 插入与删除并发，
 *              删除的记录同时匹配已提交记录及其他事务插入的记录，删除走索引，事务提交，过程中读
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "ru")
public class Transaction17775A extends SdbTestBase {

    private String clName = "transCL_17775A";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl = null;
    private BSONObject data = null;
    private BSONObject data2 = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
        expDataList = new ArrayList< BSONObject >();

        data = new BasicBSONObject();
        data.put( "_id", "insertID17775_2" );
        data.put( "a", 1 );
        data.put( "b", 1 );
        data.put( "c", 13700000000L );
        data.put( "d", "customer transaction type data application." );
        cl.insert( data );

        data2 = new BasicBSONObject();
        data2.put( "_id", "insertID17775_1" );
        data2.put( "a", 2 );
        data2.put( "b", 2 );
        data2.put( "c", 13700000000L );
        data2.put( "d", "customer transaction type data application." );

    }

    @Test
    public void test() throws InterruptedException {
        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb1 );
        TransUtils.beginTransaction( sdb2 );
        TransUtils.beginTransaction( sdb3 );

        // 2 trans1 insert data2
        cl1.insert( data2 );

        // 3 trans2 delete r1 and r2
        DeleteThread deleteThread = new DeleteThread();
        deleteThread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                deleteThread.getTransactionID() ) );

        // 4 no trans read
        expDataList.clear();
        expDataList.add( data2 );
        recordCur = cl.query( null, null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 5 trans1 read
        expDataList.clear();
        expDataList.add( data2 );
        recordCur = cl1.query( null, null, "{a: 1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl1.query( null, null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 6 trans3 read
        recordCur = cl3.query( null, null, "{a: 1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl3.query( null, null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // trans1 commit check trans2 success
        sdb1.commit();
        Assert.assertTrue( deleteThread.isSuccess(),
                deleteThread.getErrorMsg() );

        // 7 no trans read
        Assert.assertEquals( cl.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", null ) ), 0 );
        Assert.assertEquals( cl.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", "a" ) ), 0 );

        // 8 trans2 read
        Assert.assertEquals( cl2.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", null ) ), 0 );
        Assert.assertEquals( cl2.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", "a" ) ), 0 );

        // 9 trans3 read
        Assert.assertEquals( cl3.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", null ) ), 0 );
        Assert.assertEquals( cl3.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", "a" ) ), 0 );

        // 10 read after trans2 commit
        sdb2.commit();
        Assert.assertEquals( cl.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", null ) ), 0 );
        Assert.assertEquals( cl.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", "a" ) ), 0 );

        // 11 trans3 read
        Assert.assertEquals( cl3.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", null ) ), 0 );
        Assert.assertEquals( cl3.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$isnull", 0 ) ),
                new BasicBSONObject( "", "a" ) ), 0 );

        sdb3.commit();
    }

    @AfterClass
    public void tearDown() {
        sdb1.commit();
        sdb2.commit();
        sdb3.commit();

        sdb.getCollectionSpace( csName ).dropCollection( clName );
        if ( recordCur != null ) {
            recordCur.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
        if ( sdb1 != null ) {
            sdb1.close();
        }
        if ( sdb2 != null ) {
            sdb2.close();
        }
        if ( sdb3 != null ) {
            sdb3.close();
        }
    }

    private class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            cl2.delete( null, "{'': 'a'}" );
        }
    }

}
