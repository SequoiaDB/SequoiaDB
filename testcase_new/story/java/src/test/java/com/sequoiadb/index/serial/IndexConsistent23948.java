package com.sequoiadb.index.serial;

import java.util.ArrayList;
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
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23948 :: 并发创建索引和rename主表所在cl
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23948 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl = null;
    private String csName = "cs_23948";
    private String mainclName = "maincl_23948";
    private String subclName1 = "subcl_23948a";
    private String subclName2 = "subcl_23948b";
    private String newMainCLName = "newMainCL_23948";
    private int recsNum = 40000;
    ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );
        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23948";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        RenameCL renameCL = new RenameCL();
        es.addWorker( createIndex );
        es.addWorker( renameCL );
        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                Assert.fail( "---createIndex fail! the error code = "
                        + createIndex.getRetCode() );
            }
            Assert.assertFalse( cs.isCollectionExist( mainclName ) );
            reCreateIndexAndCheckResult( sdb, csName, newMainCLName, subclName1,
                    subclName2, indexName );
        } else if ( renameCL.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            Assert.assertTrue( cs.isCollectionExist( mainclName ) );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainclName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, mainclName, indexName,
                    true );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCL.getRetCode(), 0 );
            Assert.assertFalse( cs.isCollectionExist( mainclName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, mainclName );
            IndexUtils.checkIndexTask( sdb, "Create index", csName,
                    newMainCLName, indexName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;

        private CreateIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class RenameCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms内时间再rename
                int waitTime = new Random().nextInt( 500 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                cs.renameCollection( mainclName, newMainCLName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void reCreateIndexAndCheckResult( Sequoiadb db, String csName,
            String mainclName, String subclName1, String subclName2,
            String indexName ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( mainclName );
        dbcl.createIndex( indexName, "{testa:1}", false, false );
        IndexUtils.checkIndexTaskResult( db, "Create index", csName, mainclName,
                indexName, 0 );
        IndexUtils.checkIndexTaskResult( db, "Create index", csName, subclName1,
                indexName, 0 );
        IndexUtils.checkIndexTaskResult( db, "Create index", csName, subclName2,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( db, csName, subclName1, indexName,
                true );
        IndexUtils.checkIndexConsistent( db, csName, subclName2, indexName,
                true );

        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1, ( BSONObject ) JSON
                .parse( "{ShardingKey:{no:1},AutoSplit:true}" ) );
        cs.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
        return mainCL;
    }
}