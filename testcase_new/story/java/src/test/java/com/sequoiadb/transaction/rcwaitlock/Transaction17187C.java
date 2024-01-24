package com.sequoiadb.transaction.rcwaitlock;

/**
 * @Description seqDB-17187:  更新复合索引，同时与读并发
 * @author xiaoni Zhao
 * @date 2019-1-23
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rcwaitlock")
public class Transaction17187C extends SdbTestBase {
    private String clName = "cl_17187C";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCursor cursor = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl.createIndex( "a", "{a:1, b:1}", false, false );

        BSONObject insertR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1}" );
        cl.insert( insertR1 );
    }

    @Test
    public void test() throws InterruptedException {
        // 开启事务1
        TransUtils.beginTransaction( db1 );

        // 事务1新增部分索引字段
        cl1.update( null, "{$set:{b:1}}", "{'':'a'}" );
        BSONObject updateR1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        expList.add( updateR1 );

        // 事务1表扫描记录
        cursor = cl1.query( null, null, null, "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 事务1索引扫描记录
        cursor = cl1.query( null, null, null, "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

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

        // 非事务表扫描记录
        cursor = cl.query( null, null, null, "{'':null}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

        // 非事务索引扫描记录
        cursor = cl.query( null, null, null, "{'':'a'}" );
        Assert.assertEquals( TransUtils.getReadActList( cursor ), expList );

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

        cursor.close();
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
                cursor = cl2.query( null, null, null, hint );
                List< BSONObject > records = TransUtils
                        .getReadActList( cursor );
                setExecResult( records );

                // 事务2扫描记录
                cursor = cl2.query( null, null, null, hint );
                Assert.assertEquals( TransUtils.getReadActList( cursor ),
                        expList );

                // 非事务扫描记录
                cursor = cl.query( null, null, null, hint );
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
