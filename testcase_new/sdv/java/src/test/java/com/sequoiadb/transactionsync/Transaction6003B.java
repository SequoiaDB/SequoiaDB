package com.sequoiadb.transactionsync;

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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-6003:配置事务锁超时时间值合法校验_SD.transaction.014(
 *              由于设置事务锁等待超时时间值为3600s时间较长不适合将用例放到CI,故这里只测试设置为30s，看参数是否生效)
 * @author wangkexin
 * @date 2019.04.08
 * @review
 */
public class Transaction6003B extends SdbTestBase {
    private String clName = "cl6003B";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private long timeoutMillis = 0;
    private int transTimeout = 30;
    private int transDefaultTimeout = 60;
    private int tolerance = 5;

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "transactiontimeout", transTimeout );
        options.put( "Global", true );
        sdb.updateConfig( configs, options );
        checkConfig( options, transTimeout );

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new TransInsert6003B() );
        es.addWorker( new TransDelete6003B() );
        es.run();

        CheckResult();
    }

    @AfterClass
    private void teardown() {
        try {
            BSONObject configs = new BasicBSONObject();
            BSONObject options = new BasicBSONObject();
            configs.put( "transactiontimeout", transDefaultTimeout );
            options.put( "Global", true );
            sdb.updateConfig( configs, options );

            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } finally {
            if ( sdb != null )
                sdb.close();
            if ( db1 != null )
                db1.close();
            if ( db2 != null )
                db2.close();
        }
    }

    class TransInsert6003B {
        private DBCollection cl1 = null;

        public TransInsert6003B() {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 1, desc = "开启事务")
        private void beginTrans() {
            db1.beginTransaction();
        }

        @ExecuteOrder(step = 2, desc = "插入数据")
        public void Insert() {
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", 1 );
            cl1.insert( obj );
        }

        @ExecuteOrder(step = 4, desc = "提交事务")
        public void Commit() {
            db1.commit();
        }
    }

    class TransDelete6003B {
        private DBCollection cl2 = null;

        public TransDelete6003B() {
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
        }

        @ExecuteOrder(step = 1, desc = "开启事务")
        private void beginTrans() {
            db2.beginTransaction();
        }

        @ExecuteOrder(step = 3, desc = "删除数据")
        public void Delete() {
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "a", 1 );
            long starttime = System.currentTimeMillis();
            long endtime = 0;
            try {
                cl2.delete( matcher );
                Assert.fail( "exp fail but found succ." );
            } catch ( BaseException e ) {
                endtime = System.currentTimeMillis();
                Assert.assertEquals( e.getErrorCode(), -13 );
            }
            timeoutMillis = endtime - starttime;
        }

        @ExecuteOrder(step = 5, desc = "提交事务")
        public void Commit() {
            db2.commit();
        }
    }

    private void CheckResult() {
        long expCount = 1;
        long actCount = cl.getCount();
        Assert.assertEquals( actCount, expCount );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ) {
            Assert.assertEquals( cursor.getNext().get( "a" ).toString(), "1" );
        }
        if ( timeoutMillis < ( transTimeout - tolerance ) * 1000
                || timeoutMillis > ( transTimeout + tolerance ) * 1000 ) {
            Assert.fail( "when transactiontimeout is 30(s), the actual time is "
                    + timeoutMillis + "(ms)." );
        }
    }

    private void checkConfig( BSONObject options, int expValue ) {
        BSONObject selector = new BasicBSONObject();
        selector.put( "transactiontimeout", 1 );

        DBCursor cursor1 = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, options,
                selector, null );
        while ( cursor1.hasNext() ) {
            int actValue = ( int ) cursor1.getNext()
                    .get( "transactiontimeout" );
            Assert.assertEquals( actValue, expValue );
        }
    }
}