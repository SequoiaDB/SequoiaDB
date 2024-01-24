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
 * @description seqDB-6003:配置事务锁超时时间值合法校验_SD.transaction.014
 * @author wangkexin
 * @date 2019.04.08
 * @review
 */

public class Transaction6003A extends SdbTestBase {
    private String clName = "cl6003A";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private long timeoutMillis = 0;
    private int lowBoundValue = 0;
    private int upBoundValue = 3600;

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // test transactiontimeout is 0
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "transactiontimeout", lowBoundValue );
        options.put( "Global", true );
        sdb.updateConfig( configs, options );
        checkConfig( options, lowBoundValue );

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new TransInsert6003A() );
        es.addWorker( new TransDelete6003A() );
        es.run();
        CheckResult();

        // test transactiontimeout is 3599
        BSONObject configs2 = new BasicBSONObject();
        configs2.put( "transactiontimeout", upBoundValue - 1 );
        sdb.updateConfig( configs2, options );
        checkConfig( options, upBoundValue - 1 );

        // test transactiontimeout is 3600
        BSONObject configs3 = new BasicBSONObject();
        configs3.put( "transactiontimeout", upBoundValue );
        sdb.updateConfig( configs3, options );
        checkConfig( options, upBoundValue );
    }

    @AfterClass
    private void teardown() {
        try {
            // 恢复环境
            BSONObject configs = new BasicBSONObject();
            BSONObject options = new BasicBSONObject();
            configs.put( "transactiontimeout", 60 );
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

    class TransInsert6003A {
        private DBCollection cl1 = null;

        public TransInsert6003A() {
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

    class TransDelete6003A {
        private DBCollection cl2 = null;

        public TransDelete6003A() {
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
        if ( timeoutMillis > 5000 ) {
            Assert.fail( "when transactiontimeout is 0(s), the actual time is "
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