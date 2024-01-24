package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.Random;

/**
 * @Description seqDB-23951:并发删除索引和删除cs
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23951 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23951";
    private String clName1 = "cl_23951_1";
    private String clName2 = "cl_23951_2";
    private String idxName = "idx23951";
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        DBCollection cl1 = cs.createCollection( clName1 );
        cl1.createIndex( idxName, new BasicBSONObject( "testno", 1 ), false,
                false );
        DBCollection cl2 = cs.createCollection( clName2 );
        cl2.createIndex( idxName, new BasicBSONObject( "testno", 1 ), false,
                false );
        int recordNum = 15000;
        IndexUtils.insertDataWithOutReturn( cl1, recordNum );
        IndexUtils.insertDataWithOutReturn( cl2, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 两个cl并发删除索引，同时并发删除cs
        ThreadExecutor te = new ThreadExecutor();
        te.addWorker( new DropIndex( clName1 ) );
        te.addWorker( new DropIndex( clName2 ) );
        te.addWorker( new DropCS() );
        te.run();

        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, clName1 );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, clName2 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropIndex extends ResultStore {
        private Sequoiadb db;
        private DBCollection cl;
        private String clName;

        public DropIndex( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void dropIndex() throws InterruptedException {
            try {
                // 随机等待50ms再删除索引
                Random random = new Random();
                Thread.sleep( random.nextInt( 50 ) );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCS extends ResultStore {
        @ExecuteOrder(step = 2)
        private void dropCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            }
        }
    }
}
