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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17163:插入相同的记录与读并发,事务回滚
 * @date 2019-1-22
 * @author yinzhen
 *
 */
@Test(groups = "rcwaitlock")
public class Transaction17163B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "ixscan17163";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17163", "{a:1}", false, false );
        BSONObject record = ( BSONObject ) JSON.parse( "{a:1, b:1}" );
        cl.insert( record );
        expList.add( record );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void test() throws InterruptedException {
        // 开启2个并发事务
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1删除记录，并插入相同的记录
        cl1.delete( "", "{'':'textIndex17163'}" );
        BSONObject record = ( BSONObject ) JSON.parse( "{a:1, b:1}" );
        cl1.insert( record );
        List< BSONObject > expRecords = new ArrayList<>();
        expRecords.add( record );

        // 事务2读记录走索引扫描阻塞
        CL2Query cl2Thread = new CL2Query( "{a:{$exists:1}}",
                "{'':'textIndex17163'}" );
        cl2Thread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                cl2Thread.getTransactionID() ) );

        // 非事务表扫描
        DBCursor recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expRecords );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17163'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expRecords );

        // 事务1回滚
        db1.rollback();
        if ( !( cl2Thread.isSuccess() ) ) {
            Assert.fail( cl2Thread.getErrorMsg() + "\n" );
        }
        try {
            actList = ( List< BSONObject > ) cl2Thread.getExecResult();
            Assert.assertEquals( actList, expList );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        }

        // 事务2读记录走表扫描
        recordsCursor = cl2.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2读记录走索引扫描
        recordsCursor = cl2.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17163'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17163'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2回滚
        db2.rollback();
        recordsCursor.close();

        // 非事务表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17163'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );
    }

    private class CL2Query extends SdbThreadBase {
        private String hint;
        private String matcher;

        public CL2Query( String matcher, String hint ) {
            super();
            this.matcher = matcher;
            this.hint = hint;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            DBCursor cursor = cl2.query( matcher, null, null, hint );
            List< BSONObject > records = TransUtils.getReadActList( cursor );
            try {
                setExecResult( records );
            } catch ( Exception e ) {
                e.printStackTrace();
                throw e;
            }
        }
    }
}
