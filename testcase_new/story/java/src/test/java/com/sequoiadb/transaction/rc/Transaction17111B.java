package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
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
 * @testcase seqDB-17111:select for update并发读与更新并发
 * @date 2019-1-18
 * @author yinzhen
 */
@Test(groups = "rc")
public class Transaction17111B extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "ixscan17111";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'a'}";

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}" }, { "{'a': -1, 'b': 1}" }, };
    }

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
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
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    @SuppressWarnings("unchecked")
    @Test(dataProvider = "index")
    public void test( String indexKey ) throws InterruptedException {
        try {
            cl.createIndex( "a", indexKey, false, false );

            BSONObject R1 = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl.insert( R1 );
            expList.clear();
            expList.add( R1 );

            // 开启并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 事务1 select for update读记录走表扫描
            DBCursor recordsCursor = cl1.query( null, null, null, hintTbScan,
                    DBQuery.FLG_QUERY_FOR_UPDATE );
            actList = TransUtils.getReadActList( recordsCursor );
            Assert.assertEquals( actList, expList );

            // 事务2 select for update 读记录走索引扫描阻塞
            CL2Query cl2Thread = new CL2Query( null, hintIxScan );
            cl2Thread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    cl2Thread.getTransactionID() ) );

            // 事务3更新记录阻塞
            CL3Update cl3Update = new CL3Update();
            cl3Update.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    cl3Update.getTransactionID() ) );

            // 提交事务1事务2返回
            db1.commit();
            Assert.assertTrue( cl2Thread.isSuccess(), cl2Thread.getErrorMsg() );
            try {
                actList = ( List< BSONObject > ) cl2Thread.getExecResult();
                Assert.assertEquals( actList, expList );
            } catch ( InterruptedException e ) {
                Assert.fail( e.getMessage() );
            }
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    cl3Update.getTransactionID() ) );

            // 非事务表扫描
            TransUtils.queryAndCheck( cl, hintTbScan, expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, hintIxScan, expList );

            // 非事务逆序表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan, expList );

            // 非事务逆序索引扫描
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan, expList );

            // 提交事务2
            db2.commit();
            Assert.assertTrue( cl3Update.isSuccess(), cl3Update.getErrorMsg() );

            // 非事务表扫描
            BSONObject record = ( BSONObject ) JSON
                    .parse( "{_id:1, a:4, b:1}" );
            expList.clear();
            expList.add( record );
            TransUtils.queryAndCheck( cl, hintTbScan, expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, hintIxScan, expList );

            // 非事务逆序表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan, expList );

            // 非事务逆序索引扫描
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan, expList );

            // 事务3表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, hintTbScan, expList );

            // 事务3索引扫描
            TransUtils.queryAndCheck( cl3, hintIxScan, expList );

            // 事务3逆序表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintTbScan, expList );

            // 事务3逆序索引扫描
            TransUtils.queryAndCheck( cl3, "{a: -1}", hintIxScan, expList );

            // 提交事务3
            db3.commit();

            // 非事务表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, hintTbScan, expList );

            // 非事务索引扫描
            TransUtils.queryAndCheck( cl, hintIxScan, expList );

            // 非事务逆序表扫描
            Collections.reverse( expList );
            TransUtils.queryAndCheck( cl, "{a: -1}", hintTbScan, expList );

            // 非事务逆序索引扫描
            TransUtils.queryAndCheck( cl, "{a: -1}", hintIxScan, expList );
            recordsCursor.close();
        } finally {
            db1.commit();
            db2.commit();
            db3.commit();
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }

    private class CL3Update extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl3.getSequoiadb() );

            cl3.update( "{a:1}", "{$set:{a:4}}", "{'':'textIndex17111'}" );
        }
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
            setExecResult( records );
        }
    }
}
