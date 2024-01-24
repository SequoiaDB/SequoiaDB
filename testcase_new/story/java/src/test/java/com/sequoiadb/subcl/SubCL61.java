package com.sequoiadb.subcl;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-61:主子表在写入数据过程中detach子表
 * @author huangxiaoni
 * @date 2019.3.21
 * @review
 */

public class SubCL61 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private final static String MAINCL_NAME = "mcl_61";
    private final static String SUBCL_NAME_1 = "scl_61_1";
    private final static String SUBCL_NAME_2 = "scl_61_2";
    private final static int RECORDS_NUM = 10000;
    // number of records num of subCL2 when subCL2 be dettach
    private int subCL2Num = 0;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "The mode is standlone." );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection mCL = cs.createCollection( MAINCL_NAME, ( BSONObject ) JSON
                .parse( "{IsMainCL:true, ShardingKey:{a:1}}" ) );
        DBCollection sCL1 = cs.createCollection( SUBCL_NAME_1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{a:1}}" ) );
        DBCollection sCL2 = cs.createCollection( SUBCL_NAME_2,
                ( BSONObject ) JSON.parse( "{ShardingKey:{a:1}}" ) );
        mCL.attachCollection( sCL1.getFullName(), ( BSONObject ) JSON
                .parse( "{LowBound:{a:0}, UpBound:{a:1}}" ) );
        mCL.attachCollection( sCL2.getFullName(), ( BSONObject ) JSON
                .parse( "{LowBound:{a:1}, UpBound:{a:2}}" ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new insertToSubCL1() );
        es.addWorker( new insertToSubCL2() );
        es.addWorker( new detachSubCL() );
        es.run();
    }

    @AfterClass
    private void tearDown() {
        try {
            cs.dropCollection( MAINCL_NAME );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class insertToSubCL1 {
        @ExecuteOrder(step = 1)
        private void insert() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection mCL = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( MAINCL_NAME );
                for ( int i = 0; i < RECORDS_NUM; i++ ) {
                    BSONObject insertor = new BasicBSONObject();
                    insertor.put( "a", 0 );
                    insertor.put( "b", i );
                    mCL.insert( insertor );
                }
            } finally {
                if ( db != null )
                    db.close();
            }
        }

        @ExecuteOrder(step = 2)
        private void checkResult() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection mCL = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( MAINCL_NAME );
                BSONObject cntOpt = new BasicBSONObject();
                cntOpt.put( "a", 0 );
                long cnt = mCL.getCount( cntOpt );
                Assert.assertEquals( cnt, RECORDS_NUM );
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }

    private class insertToSubCL2 {
        @ExecuteOrder(step = 1)
        private void insert() {
            Sequoiadb db = null;
            DBCollection mCL = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                mCL = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( MAINCL_NAME );
                for ( subCL2Num = 0; subCL2Num < RECORDS_NUM; subCL2Num++ ) {
                    BSONObject insertor = new BasicBSONObject();
                    insertor.put( "a", 1 );
                    insertor.put( "b", subCL2Num );
                    mCL.insert( insertor );
                }
            } catch ( BaseException e ) {
                if ( -135 != e.getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null )
                    db.close();
            }
        }

        @ExecuteOrder(step = 2)
        private void checkResult() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection mCL = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( MAINCL_NAME );
                BasicBSONObject cntOpt = new BasicBSONObject();
                cntOpt.put( "a", 1 );
                Assert.assertEquals( mCL.getCount( cntOpt ), 0 );

                DBCollection sCL2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( SUBCL_NAME_2 );
                Assert.assertEquals( sCL2.getCount( cntOpt ), subCL2Num );
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }

    private class detachSubCL {
        @ExecuteOrder(step = 1)
        public void detachCL() throws InterruptedException {
            Random random = new Random();
            Thread.sleep( random.nextInt( 1000 ) );
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection mCL = cs.getCollection( MAINCL_NAME );
                mCL.detachCollection(
                        cs.getCollection( SUBCL_NAME_2 ).getFullName() );
            } finally {
                if ( db != null )
                    db.close();
            }
        }
    }
}
