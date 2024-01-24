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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17182:select for update并发读与更新并发
 * @date 2019-1-22
 * @author yinzhen
 *
 */
@Test(groups = "rcwaitlock")
public class Transaction17182B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "ixsacn17182";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "textIndex17182", "{a:1}", false, false );
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
        cl.insert( record );
        expList.add( record );
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

    @SuppressWarnings("unchecked")
    @Test
    public void test() throws InterruptedException {
        // 开启并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        TransUtils.beginTransaction( db3 );
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

        // 事务1 select for update读记录走表扫描
        DBCursor recordsCursor = cl1.query( null, null, null, "{'':null}",
                DBQuery.FLG_QUERY_FOR_UPDATE );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2 select for update读记录走索引扫描阻塞
        CL2Query cl2Thread = new CL2Query( "{a:{$exists:1}}",
                "{'':'textIndex17182'}" );
        cl2Thread.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                cl2Thread.getTransactionID() ) );

        // 事务3更新记录阻塞
        CL3Update cl3Update = new CL3Update();
        cl3Update.start();
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                cl3Update.getTransactionID() ) );

        // 提交事务1
        db1.commit();
        if ( !( cl2Thread.isSuccess() ) ) {
            Assert.fail( cl2Thread.getErrorMsg() + "\n" );
        }
        try {
            actList = ( List< BSONObject > ) cl2Thread.getExecResult();
            Assert.assertEquals( actList, expList );
        } catch ( InterruptedException e ) {
            Assert.fail( e.getMessage() );
        }
        Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                cl3Update.getTransactionID() ) );

        // 非事务表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17182'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 提交事务2
        db2.commit();
        if ( !cl3Update.isSuccess() ) {
            Assert.fail( cl3Update.getErrorMsg() );
        }

        // 非事务表扫描
        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1, a:4, b:1}" );
        expList.clear();
        expList.add( record );
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17182'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务3读走表扫描
        recordsCursor = cl3.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务3读走索引扫描
        recordsCursor = cl3.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17183'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 提交事务3
        db3.commit();

        // 非事务表扫描
        recordsCursor = cl.query( null, null, null, "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 非事务索引扫描
        recordsCursor = cl.query( "{a:{$exists:1}}", null, null,
                "{'':'textIndex17182'}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );
        recordsCursor.close();
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

            DBCursor cursor = cl2.query( matcher, null, null, hint,
                    DBQuery.FLG_QUERY_FOR_UPDATE );
            List< BSONObject > records = TransUtils.getReadActList( cursor );
            try {
                setExecResult( records );
            } catch ( Exception e ) {
                e.printStackTrace();
                throw e;
            }
        }
    }

    private class CL3Update extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl3.getSequoiadb() );

            cl3.update( "{a:1}", "{$set:{a:4}}", "{'':'textIndex17182'}" );
        }
    }
}
