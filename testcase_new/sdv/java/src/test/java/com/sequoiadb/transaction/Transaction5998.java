package com.sequoiadb.transaction;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-5998:多个事务并发，同时向相同cl插入数据并提交事务
 * @author huangxiaoni
 * @date 2019.3.18
 * @review
 */

public class Transaction5998 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private CollectionSpace cs;
    private DBCollection cl;
    private final static String CL_NAME = "trans5998";
    private final static int DOCS_NUM = 10000;
    private final static int REPEAT_BEGIN_NUM = 10;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( CL_NAME );
    }

    @Test()
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new trans1() );
        es.addWorker( new trans2() );
        es.run();

        // check results
        Assert.assertEquals( DOCS_NUM * 2, cl.getCount() );

        long cnt1 = cl.getCount(
                new BasicBSONObject( "a", new BasicBSONObject( "$lt", 10 ) ) );
        Assert.assertEquals( REPEAT_BEGIN_NUM, cnt1 );

        long cnt2 = cl.getCount( new BasicBSONObject( "a",
                new BasicBSONObject( "$gte", DOCS_NUM ) ) );
        Assert.assertEquals( REPEAT_BEGIN_NUM, cnt2 );

        ArrayList< BSONObject > andArr = new ArrayList<>();
        andArr.add( new BasicBSONObject( "a",
                new BasicBSONObject( "$gte", REPEAT_BEGIN_NUM ) ) );
        andArr.add( new BasicBSONObject( "a",
                new BasicBSONObject( "$lt", DOCS_NUM ) ) );
        long cnt3 = cl.getCount( new BasicBSONObject( "$and", andArr ) );
        Assert.assertEquals( ( DOCS_NUM * 2 ) - ( cnt1 + cnt2 ), cnt3 );
    }

    @AfterClass
    private void tearDown() {
        try {
            cs.dropCollection( CL_NAME );
        } finally {
            if ( sdb != null )
                sdb.close();
            if ( db1 != null )
                db1.close();
            if ( db2 != null )
                db2.close();
        }
    }

    private class trans1 {
        private DBCollection cl;

        private trans1() {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1)
        private void beginTrans() {
            db1.beginTransaction();
        }

        @ExecuteOrder(step = 2)
        private void update() {
            ArrayList< BSONObject > insertor = new ArrayList<>();
            for ( int i = 0; i < DOCS_NUM; i++ ) {
                insertor.add( new BasicBSONObject( "a", i ) );
            }
            cl.insert( insertor );
        }

        @ExecuteOrder(step = 3)
        private void endTrans() {
            db1.commit();
        }
    }

    private class trans2 {
        private DBCollection cl;

        private trans2() {
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 1)
        private void beginTrans() {
            db2.beginTransaction();
        }

        @ExecuteOrder(step = 2)
        private void update() {
            ArrayList< BSONObject > insertor = new ArrayList<>();
            for ( int i = REPEAT_BEGIN_NUM; i < DOCS_NUM
                    + REPEAT_BEGIN_NUM; i++ ) {
                insertor.add( new BasicBSONObject( "a", i ) );
            }
            cl.insert( insertor );
        }

        @ExecuteOrder(step = 3)
        private void endTrans() {
            db2.commit();
        }
    }
}
