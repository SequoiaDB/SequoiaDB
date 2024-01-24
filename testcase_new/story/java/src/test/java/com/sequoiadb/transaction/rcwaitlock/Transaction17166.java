package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17166:   事务中批量增删改操作与读并发 
 * @author xiaoni Zhao
 * @date 2019-1-23
 */
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17166 extends SdbTestBase {
    private String clName = "cl_17166";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void test() throws InterruptedException {
        // 开启事务1,执行插入操作
        TransUtils.beginTransaction( db1 );
        expList = TransUtils.insertRandomDatas( cl1, 0, 500 );

        // 非事务表扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'a'}", expList );

        // 事务2表扫描记录
        Read read1 = new Read( "{'':null}" );
        read1.start();

        // 事务2索引扫描记录
        Read read2 = new Read( "{'':'a'}" );
        read2.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read1.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read2.getTransactionID() ) );

        db1.commit();

        // 校验被阻塞线程返回的记录
        if ( !read1.isSuccess() || !read2.isSuccess() ) {
            Assert.fail( read1.getErrorMsg() + read2.getErrorMsg() );
        }
        try {
            Assert.assertEquals( read1.getExecResult(), expList );
            Assert.assertEquals( read2.getExecResult(), expList );
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() );
        }

        // 开启事务1,执行更新操作
        TransUtils.beginTransaction( db1 );
        cl1.update( "", "{$inc:{a:1}}", "{'':'a'}" );

        // 非事务表扫描记录
        expList.clear();
        expList = TransUtils.getIncDatas( 0, 500, 1 );
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'a'}", expList );

        // 事务2表扫描记录
        Read read3 = new Read( "{'':null}" );
        read3.start();

        // 事务2索引扫描记录
        Read read4 = new Read( "{'':'a'}" );
        read4.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read3.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read4.getTransactionID() ) );

        db1.commit();

        // 校验被阻塞线程返回的记录
        if ( !read3.isSuccess() || !read3.isSuccess() ) {
            Assert.fail( read3.getErrorMsg() + read4.getErrorMsg() );
        }
        try {
            Assert.assertEquals( read3.getExecResult(), expList );
            Assert.assertEquals( read4.getExecResult(), expList );
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() );
        }

        // 开启事务1,执行删除操作
        TransUtils.beginTransaction( db1 );
        cl1.delete( "", "{'':'a'}" );

        // 非事务表扫描记录
        expList.clear();
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':null}", expList );

        // 非事务索引扫描记录
        TransUtils.queryAndCheck( cl, "{_id:1}", "{'':'a'}", expList );

        // 事务2表扫描记录
        Read read5 = new Read( "{'':null}" );
        read5.start();

        // 事务2索引扫描记录
        Read read6 = new Read( "{'':'a'}" );
        read6.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read5.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, read6.getTransactionID() ) );

        db1.commit();

        // 校验被阻塞线程返回的记录
        if ( !read5.isSuccess() || !read6.isSuccess() ) {
            Assert.fail( read5.getErrorMsg() + read6.getErrorMsg() );
        }
        try {
            Assert.assertEquals( read5.getExecResult(), expList );
            Assert.assertEquals( read6.getExecResult(), expList );
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() );
        }

    }

    private class Read extends SdbThreadBase {
        private Sequoiadb db = null;
        private Sequoiadb db2 = null;
        private DBCollection cl = null;
        private DBCollection cl2 = null;
        private String hint = null;
        private DBCursor cursor = null;

        public Read( String hint ) {
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );

            // 开启并发事务2
            TransUtils.beginTransaction( db2 );

            // 判断事务阻塞需先获取事务id
            setTransactionID( db2 );

            try {
                cursor = cl2.query( null, null, "{_id:1}", hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                setExecResult( records );

                // 事务2扫描记录
                cursor = cl2.query( null, null, "{_id:1}", hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        expList );

                // 非事务扫描记录
                cursor = cl.query( null, null, "{_id:1}", hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        expList );

                db2.commit();
            } finally {
                db2.commit();
                cursor.close();
                db2.close();
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        if ( !db1.isClosed() ) {
            db1.close();
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
