package com.sequoiadb.index;

import java.util.Date;

import org.bson.BasicBSONObject;
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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-20557:创建集合过程中插入_id重复的记录
 * @Author HuangXiaoNi 2020/03/08
 */

public class IdIndex20557 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String csName = "idIndex20557";
    private String clNameBase = "idIndex20557_";
    private int threadNum = 100;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        // 独立模式并发上不去，大并发下insertRecs容易报-15/-23，跑了没有意义，跳过独立模式
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip standAlone mode" );
        }

        CommLib.clearCS( sdb, csName );
        cs = sdb.createCollectionSpace( csName );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CreateCL( clNameBase + i ) );
            es.addWorker( new InsertRecs( clNameBase + i ) );
        }
        es.run();

        checkRecs();

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateCL {
        private String clName;

        private CreateCL( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void createCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.createCollection( clName );
            }
        }
    }

    private class InsertRecs extends ResultStore {
        private String clName;

        private InsertRecs( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void insertRecs() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );

                boolean isCLExist = false;
                long maxTimeout = 1 * 60 * 1000;
                long currTime1 = new Date().getTime();
                long currTime2 = new Date().getTime();
                while ( ( currTime2 - currTime1 ) <= maxTimeout ) {
                    if ( cs.isCollectionExist( clName ) ) {
                        isCLExist = true;
                        break;
                    }
                    currTime2 = new Date().getTime();
                }
                if ( !isCLExist ) {
                    throw new Exception( clName + " not exist, maxTimeout = "
                            + maxTimeout + ", actTimeout = "
                            + ( currTime2 - currTime1 ) + ", currTime2 = "
                            + currTime2 + ", currTime1 = " + currTime1 );
                }

                DBCollection cl = cs.getCollection( clName );
                cl.insert( new BasicBSONObject( "_id", 1 ) );
                try {
                    cl.insert( new BasicBSONObject( "_id", 1 ) );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -38 ) {
                        throw e;
                    }
                }
            }
        }
    }

    private void checkRecs() {
        for ( int i = 0; i < threadNum; i++ ) {
            // check idIndex
            DBCollection cl = cs.getCollection( clNameBase + i );
            Assert.assertTrue( cl.isIndexExist( "$id" ) );

            // check records
            Assert.assertEquals( cl.getCount(), 1 );
        }
    }
}
