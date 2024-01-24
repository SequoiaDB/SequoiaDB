package com.sequoiadb.index;

import java.util.Date;
import java.util.Random;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23941:并发创建索引和删除子表(删除其中一个子表)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23941A extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private String mainclName = "maincl_23941a";
    private String subclName1 = "subcl_23941a_1";
    private String subclName2 = "subcl_23941a_2";
    private int recsNum = 40000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        dbcl = createAndAttachCL( cs, mainclName, subclName1, subclName2 );
        IndexUtils.insertDataWithOutReturn( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23941A";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        DropSubCL dropSubCL = new DropSubCL( subclName1 );
        es.addWorker( createIndex );
        es.addWorker( dropSubCL );
        es.run();

        // dropCL可能加锁失败报错-147
        if ( dropSubCL.getRetCode() != 0 ) {
            Assert.assertEquals( dropSubCL.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    subclName1, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, true );
        } else {
            Assert.assertFalse( cs.isCollectionExist( subclName1 ) );
            IndexUtils.checkNoTask( sdb, "Create index", SdbTestBase.csName,
                    subclName1 );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, false );
        }

        if ( createIndex.getRetCode() != SDBError.SDB_TASK_HAS_CANCELED.getErrorCode() ) {
           Assert.assertEquals( createIndex.getRetCode(), 0 );
           IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                   mainclName, indexName );
           IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                   subclName2, indexName );
           IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                   indexName, true );
           Assert.assertTrue( dbcl.isIndexExist( indexName ),
                   "check maincl index=" + indexName );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( mainclName );
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
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropSubCL extends ResultStore {
        private String clName;

        private DropSubCL( String clName ) {
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待 1S内时间再删除cl
                int waitTime = new Random().nextInt( 1000 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( subclName1 );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private DBCollection createAndAttachCL( CollectionSpace cs,
            String mainclName, String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
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
