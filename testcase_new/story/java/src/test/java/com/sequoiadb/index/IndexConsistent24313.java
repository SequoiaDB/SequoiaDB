package com.sequoiadb.index;

import java.util.Date;
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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-24313 :: 并发创建索引和删除主表所在cs，其中主表和子表不在同一cs
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent24313 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs1;
    private CollectionSpace cs2;
    private DBCollection dbcl = null;
    private String csName1 = "cs_24313a";
    private String csName2 = "cs_24313b";
    private String mainclName = "maincl_24313";
    private String subclName1 = "subcl_24313a";
    private String subclName2 = "subcl_24313b";
    private int recsNum = 40000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }

        cs1 = sdb.createCollectionSpace( csName1 );
        cs2 = sdb.createCollectionSpace( csName2 );
        dbcl = createAndAttachCL( cs1, cs2, mainclName, subclName1,
                subclName2 );
        IndexUtils.insertDataWithOutReturn( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex24313";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        DropCS dropCS = new DropCS( csName1 );
        es.addWorker( createIndex );
        es.addWorker( dropCS );
        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            // create fail drop cs success
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_DELETING
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_TASK_HAS_CANCELED
                            .getErrorCode() ) {
                Assert.fail( "---errorCode=" + createIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName1 ) );
            boolean isExistTask = IndexUtils.isExistTask( sdb, "Create index",
                    csName1, mainclName );
            if ( isExistTask ) {
                IndexUtils.checkIndexTask( sdb, "Create index", csName1,
                        mainclName, indexName,
                        SDBError.SDB_DMS_NOTEXIST.getErrorCode() );
            }
            Assert.assertTrue( cs2.isCollectionExist( subclName2 ) );
        } else if ( dropCS.getRetCode() != 0 ) {
            // create success and drop cs fail
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", csName1, mainclName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName1, subclName1,
                    indexName, true );
            IndexUtils.checkIndexConsistent( sdb, csName2, subclName2,
                    indexName, true );
        } else {
            // create success and drop cs success,如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName1 ) );
            Assert.assertTrue( cs2.isCollectionExist( subclName2 ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName1, mainclName );
            IndexUtils.checkNoTask( sdb, "Create index", csName1, subclName1 );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                    sdb.dropCollectionSpace( csName1 );
                }
                if ( sdb.isCollectionSpaceExist( csName2 ) ) {
                    sdb.dropCollectionSpace( csName2 );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;
        private Sequoiadb db = null;
        private DBCollection cl = null;

        private CreateIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName1 ).getCollection( mainclName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try {
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCS extends ResultStore {
        private String csName;

        private DropCS( String csName ) {
            this.csName = csName;
        }

        @ExecuteOrder(step = 2)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms内时间再删除cl
                int waitTime = new Random().nextInt( 500 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs1,
            CollectionSpace cs2, String mainclName, String subclName1,
            String subclName2 ) {
        cs1.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );
        cs2.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs1.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName1 + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName2 + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
        return mainCL;
    }
}
