package com.sequoiadb.transaction;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.threadexecutor.annotation.ExpectBlock;

/**
 * @description seqDB-11200:事务中update操作不更新记录，其他事务并发更新相同记录
 * @author huangxiaoni
 * @date 2019.4.9
 * @review
 */

public class Transaction11200 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private CollectionSpace cs;
    private DBCollection cl;
    private final static String CL_NAME = "trans_11200";

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( CL_NAME );
    }

    @Test()
    private void test() throws Exception {
        cl.insert( new BasicBSONObject( "a", 1 ) );

        // commit
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new trans1(
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 1 ) ),
                "commit" ) );
        es.addWorker( new trans2(
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 2 ) ),
                "commit" ) );
        es.run();

        // rollbock
        es = new ThreadExecutor();
        es.addWorker( new trans1(
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 2 ) ),
                "rollback" ) );
        es.addWorker( new trans2(
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 3 ) ),
                "rollback" ) );
        es.run();

        Assert.assertEquals( 1, cl.getCount() );
        DBCursor cr = cl.query();
        Assert.assertEquals( 2, cr.getNext().get( "a" ) );
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
        private BSONObject modifier;
        private String transEndFlag;
        private DBCollection cl;

        private trans1( BSONObject modifier, String transEndFlag ) {
            this.modifier = modifier;
            this.transEndFlag = transEndFlag;
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
            System.out.println( new Date() + " "
                    + this.getClass().getName().toString() + " step 2 update" );
            cl.update( null, modifier, null );
        }

        @ExecuteOrder(step = 5)
        private void endTrans() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString()
                            + " step 5 " + transEndFlag );
            if ( transEndFlag == "commit" ) {
                db1.commit();
            } else {
                db1.rollback();
            }
        }
    }

    private class trans2 {
        private BSONObject modifier;
        private String transEndFlag;
        private DBCollection cl;

        private trans2( BSONObject modifier, String transEndFlag ) {
            this.modifier = modifier;
            this.transEndFlag = transEndFlag;
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( CL_NAME );
        }

        @ExecuteOrder(step = 3)
        private void beginTrans() {
            db2.beginTransaction();
        }

        @ExecuteOrder(step = 4)
        @ExpectBlock(confirmTime = 3, contOnStep = 5)
        private void update() {
            System.out.println( new Date() + " "
                    + this.getClass().getName().toString() + " step 4 update" );
            cl.update( null, modifier, null );
        }

        @ExecuteOrder(step = 6)
        private void endTrans() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString()
                            + " step 6 " + transEndFlag );
            if ( transEndFlag == "commit" ) {
                db2.commit();
            } else {
                db2.rollback();
            }
        }
    }
}
