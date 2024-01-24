package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17153: 增删改记录与读记录并发，事务回滚 
 * @author xiaoni Zhao
 * @date 2019-1-22
 */
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17153 extends SdbTestBase {
    private String clName = "cl_17153";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > expList1 = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() throws InterruptedException {
        // 开启事务1
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );

        // 事务1插入记录R1
        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1,a:1,b:1}" );
        cl1.insert( insertR1 );
        expList.add( insertR1 );

        // 事务2表扫描记录
        Query read1 = new Query( cl2, "{'':null}",
                new ArrayList< BSONObject >() );
        read1.start();

        // 事务2索引扫描记录
        Query read2 = new Query( cl3, "{'':'a'}",
                new ArrayList< BSONObject >() );
        read2.start();

        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read1.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read2.getTransactionID() ) );

        // 非事务扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );

        db1.rollback();

        Assert.assertTrue( read1.isSuccess(), read1.getErrorMsg() );
        Assert.assertTrue( read2.isSuccess(), read2.getErrorMsg() );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );

        // 事务1插入记录R1
        cl1.insert( insertR1 );
        TransUtils.beginTransaction( db1 );
        cl1.update( "", "{$inc:{a:1}}", "{'':'a'}" );

        // 事务2表扫描记录
        Query read3 = new Query( cl2, "{'':null}", expList );
        read3.start();

        // 事务2索引扫描记录
        Query read4 = new Query( cl3, "{'':'a'}", expList );
        read4.start();

        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read3.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read4.getTransactionID() ) );

        // 非事务扫描记录
        expList1.add( ( BSONObject ) JSON.parse( "{_id:1,a:2,b:1}" ) );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList1 );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList1 );

        db1.rollback();

        Assert.assertTrue( read3.isSuccess(), read3.getErrorMsg() );
        Assert.assertTrue( read4.isSuccess(), read4.getErrorMsg() );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expList );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );

        // 事务1插入记录R1
        TransUtils.beginTransaction( db1 );
        cl1.delete( "", "{'':'a'}" );

        // 事务2表扫描记录
        Query read5 = new Query( cl2, "{'':null}", expList );
        read5.start();

        // 事务2索引扫描记录
        Query read6 = new Query( cl3, "{'':'a'}", expList );
        read6.start();

        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read5.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read6.getTransactionID() ) );

        // 非事务扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}",
                new ArrayList< BSONObject >() );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}",
                new ArrayList< BSONObject >() );

        db1.rollback();

        Assert.assertTrue( read5.isSuccess(), read5.getErrorMsg() );
        Assert.assertTrue( read6.isSuccess(), read6.getErrorMsg() );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':'a'}", expList );
        TransUtils.queryAndCheck( cl2, "{a:1}", "{'':null}", expList );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}", expList );
        TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList );

        // 事务2提交
        db2.commit();
        db3.commit();
    }

    private class Query extends SdbThreadBase {
        private String hint;
        private List< BSONObject > expList;
        private DBCollection cl;

        private Query( DBCollection cl, String hint,
                List< BSONObject > expList ) {
            this.cl = cl;
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            TransUtils.queryAndCheck( cl, "{a:1}", hint, expList );
        }
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
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
