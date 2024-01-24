package com.sequoiadb.transaction.rcuserbs;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @Description Transaction18237.java 不保存老版本，并发读写同一条记录，事务提交
 * @author luweikang
 * @date 2019年1月15日
 */
@Test(groups = "rcuserbs")
public class Transaction18237 extends SdbTestBase {

    private String clName = "transCL_18237";
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private Sequoiadb sdb2 = null;
    private Sequoiadb sdb3 = null;
    private Sequoiadb sdb4 = null;
    private Sequoiadb sdb5 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;
    private DBCollection cl5 = null;
    private DBCursor recordCur = null;
    private List< BSONObject > expDataList = null;
    private List< BSONObject > actDataList = null;
    private Boolean isTransisolationRR;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb4 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb5 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a: 1}", false, false );
        expDataList = new ArrayList< BSONObject >();

        cl.insert( "{'_id': 1, 'a': 1}" );
        isTransisolationRR = TransUtils.isTransisolationRR( sdb );

    }

    @Test
    public void test() throws InterruptedException {

        cl1 = sdb1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = sdb2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = sdb3.getCollectionSpace( csName ).getCollection( clName );
        cl4 = sdb4.getCollectionSpace( csName ).getCollection( clName );
        cl5 = sdb5.getCollectionSpace( csName ).getCollection( clName );

        TransUtils.beginTransaction( sdb1 );
        TransUtils.beginTransaction( sdb2 );
        TransUtils.beginTransaction( sdb3 );
        TransUtils.beginTransaction( sdb4 );
        TransUtils.beginTransaction( sdb5 );

        // 1 trans1 query
        expDataList.clear();
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 1, 'a': 1}" ) );
        recordCur = cl1.query( "{a:1}", null, "{a: 1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl1.query( "{a:1}", null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // 2 trans2 query
        recordCur = cl2.query( "{a:1}", null, "{a: 1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl2.query( "{a:1}", null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        // trans3 update R1 to R2
        cl3.update( "{a: 1}", "{$set: {a: 2}}", null );

        // trans 4 query
        QueryThread queryThread1 = new QueryThread( cl4, "{'': null}" );
        queryThread1.start();
        QueryThread queryThread2 = new QueryThread( cl5, "{'': 'a'}" );
        queryThread2.start();

        // mvcc分支下transuserbs在RR隔离级别下强制为true,查询不阻塞
        if ( !isTransisolationRR ) {
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread1.getTransactionID() ) );
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    queryThread2.getTransactionID() ) );
        } else {
            Assert.assertFalse( TransUtils.isTransWaitLock( sdb,
                    queryThread1.getTransactionID() ) );
            Assert.assertFalse( TransUtils.isTransWaitLock( sdb,
                    queryThread2.getTransactionID() ) );
        }

        // no trans read
        expDataList.clear();
        expDataList.add( ( BSONObject ) JSON.parse( "{'_id': 1, 'a': 2}" ) );
        recordCur = cl.query( null, null, "{a: 1}", "{'': null}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        recordCur = cl.query( null, null, "{a: 1}", "{'': 'a'}" );
        actDataList = TransUtils.getReadActList( recordCur );
        Assert.assertEquals( actDataList, expDataList );
        actDataList.clear();

        sdb1.commit();
        sdb2.commit();
        sdb3.commit();

        if ( !isTransisolationRR ) {
            Assert.assertTrue( queryThread1.isSuccess(),
                    queryThread1.getErrorMsg() );
            Assert.assertTrue( queryThread2.isSuccess(),
                    queryThread2.getErrorMsg() );
        }

        sdb4.commit();
        sdb5.commit();
    }

    @AfterClass
    public void tearDown() {
        sdb.commit();
        sdb1.commit();
        sdb2.commit();
        sdb3.commit();
        sdb4.commit();
        sdb5.commit();

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
        if ( sdb4 != null ) {
            sdb4.close();
        }
        if ( sdb5 != null ) {
            sdb5.close();
        }
    }

    private class QueryThread extends SdbThreadBase {

        private DBCollection cl = null;
        private String hint = null;

        public QueryThread( DBCollection cl, String hint ) {
            this.cl = cl;
            this.hint = hint;
        }

        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            List< BSONObject > expList = new ArrayList< BSONObject >();
            expList.add( ( BSONObject ) JSON.parse( "{'_id': 1, 'a': 2}" ) );

            DBCursor cur = cl.query( null, null, "{a: 1}", hint );
            List< BSONObject > actList = TransUtils.getReadActList( cur );
            Assert.assertEquals( actList, expList );
        }
    }

}
