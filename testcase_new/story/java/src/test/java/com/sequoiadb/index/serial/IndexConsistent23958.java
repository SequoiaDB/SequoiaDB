package com.sequoiadb.index.serial;

import java.util.Date;
import java.util.Random;

import com.sequoiadb.testcommon.CommLib;
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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23958:并发删除索引和rename主表所在cs
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23958 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String mainCSName = "maincs23958";
    private String newMainCSName = "newMainCS_23958";
    private String mainclName = "maincl_23958";
    private String subclName1 = "subcl_239458a";
    private String subclName2 = "subcl_239458b";
    private String indexName = "testindex23958";
    private int recsNum = 40000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( sdb.isCollectionSpaceExist( mainCSName ) ) {
            sdb.dropCollectionSpace( mainCSName );
        }
        if ( sdb.isCollectionSpaceExist( newMainCSName ) ) {
            sdb.dropCollectionSpace( newMainCSName );
        }

        sdb.createCollectionSpace( mainCSName );
        DBCollection maincl = createAndAttachCL( cs, mainclName, subclName1,
                subclName2 );
        maincl.createIndex( indexName, "{testno:1,no:-1}", true, false );
        IndexUtils.insertDataWithOutReturn( maincl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        RenameCS renameCS = new RenameCS();
        es.addWorker( dropIndex );
        es.addWorker( renameCS );
        es.run();

        // check results
        if ( dropIndex.getRetCode() != 0 ) {
            // dropindex failed and rename cs success!
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            if ( dropIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                    .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                            .getErrorCode()
                    && dropIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---drop index fail!" + dropIndex.getRetCode() );
            }

            Assert.assertFalse( sdb.isCollectionSpaceExist( mainCSName ) );
            boolean isExistTask = IndexUtils.isExistTask( sdb, "Drop index",
                    newMainCSName, mainclName );
            if ( isExistTask ) {
                int[] resultCodes = new int[] {
                        SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode(),
                        SDBError.SDB_DMS_NOTEXIST.getErrorCode() };
                IndexUtils.checkIndexTask( sdb, "Drop index", newMainCSName,
                        mainclName, indexName, resultCodes );
            }
            reDropIndexAndCheckResult( sdb, newMainCSName, mainclName,
                    subclName1, subclName2, indexName, 0 );
        } else if ( renameCS.getRetCode() != 0 ) {
            // dropindex success and rename cs fail!
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainclName,
                    indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName1, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", mainCSName,
                    subclName2, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, false );
            IndexUtils.checkIndexConsistent( sdb, mainCSName, subclName2,
                    indexName, false );
        } else {
            // dropindex and rename cs success!
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", newMainCSName,
                    mainclName, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    subclName1, indexName );
            IndexUtils.checkIndexTask( sdb, "Drop index", newMainCSName,
                    subclName2, indexName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( newMainCSName ) ) {
                    sdb.dropCollectionSpace( newMainCSName );
                }
                if ( sdb.isCollectionSpaceExist( mainCSName ) ) {
                    sdb.dropCollectionSpace( mainCSName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class DropIndex extends ResultStore {
        private Sequoiadb db = null;
        private DBCollection dbcl = null;

        @ExecuteOrder(step = 1)
        private void getcl() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( mainCSName )
                    .getCollection( mainclName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                dbcl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class RenameCS extends ResultStore {
        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待5ms内时间再rename
                int waitTime = new Random().nextInt( 5 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                db.renameCollectionSpace( mainCSName, newMainCSName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        CollectionSpace maincs = sdb.getCollectionSpace( mainCSName );
        cs.createCollection( subclName1 );
        maincs.createCollection( subclName2, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = maincs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( SdbTestBase.csName + "." + subclName1,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( mainCSName + "." + subclName2,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
        return mainCL;
    }

    private void reDropIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String subclName1, String subclName2,
            String indexName, int resultCode ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.dropIndex( indexName );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, clName,
                indexName, resultCode );
        IndexUtils.checkIndexTaskResult( db, "Drop index", SdbTestBase.csName,
                subclName1, indexName, resultCode );
        IndexUtils.checkIndexTaskResult( db, "Drop index", csName, subclName2,
                indexName, resultCode );
        IndexUtils.checkIndexConsistent( db, csName, subclName1, indexName,
                false );
        IndexUtils.checkIndexConsistent( db, csName, subclName2, indexName,
                false );
    }
}