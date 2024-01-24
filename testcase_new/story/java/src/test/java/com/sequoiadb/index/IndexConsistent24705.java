package com.sequoiadb.index;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-24705:并发创建索引和删除子表所在cs，其中主表和删除子表同一CS
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent24705 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace dropCS1;
    private CollectionSpace cs2;
    private String subcsName = "cs_24705";
    private String mainclName = "maincl_24705";
    private String subclName1 = "subcl_24705a";
    private String subclName2 = "subcl_24705b";
    private int recsNum = 40000;
    ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }

        if ( sdb.isCollectionSpaceExist( subcsName ) ) {
            sdb.dropCollectionSpace( subcsName );
        }
        dropCS1 = sdb.createCollectionSpace( subcsName );
        cs2 = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection dbcl = createAndAttachCL( dropCS1, cs2, mainclName,
                subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex24705";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        DropCS dropCS = new DropCS( subcsName );
        es.addWorker( createIndex );
        es.addWorker( dropCS );
        es.run();

        // check results
        if ( dropCS.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    mainclName, indexName );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    subclName2, indexName );
            IndexUtils.checkIndexTask( sdb, "Create index", subcsName,
                    subclName1, indexName );

        } else if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                Assert.fail( "---errorCode=" + createIndex.getRetCode() );
            }

            Assert.assertFalse( sdb.isCollectionSpaceExist( subcsName ) );
            IndexUtils.checkNoTask( sdb, "Create index", subcsName,
                    subclName1 );
            IndexUtils.checkNoTask( sdb, "Create index", subcsName,
                    mainclName );
            IndexUtils.checkNoTask( sdb, "Create index", SdbTestBase.csName,
                    subclName2 );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName2, indexName, false );
            Assert.assertTrue( cs2.isCollectionExist( subclName2 ) );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( subcsName ) );
            IndexUtils.checkNoTask( sdb, "Create index", subcsName,
                    subclName1 );
            IndexUtils.checkNoTask( sdb, "Create index", subcsName,
                    mainclName );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    subclName2, indexName );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( subcsName ) ) {
                    sdb.dropCollectionSpace( subcsName );
                }
                cs2.dropCollection( subclName2 );
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

    private class DropCS extends ResultStore {
        private String csName;

        private DropCS( String csName ) {
            this.csName = csName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待5-1000ms
                int sleeptime = new Random().nextInt( 1000 - 5 ) + 5;
                try {
                    Thread.sleep( sleeptime );
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
                .parse( "{ShardingKey:{no:1}},AutoSplit:true" ) );

        cs2.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs1.createCollection( mainclName, optionsM );

        mainCL.attachCollection( subcsName + "." + subclName1,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{no:0},UpBound:{no:30000}}" ) );
        mainCL.attachCollection( SdbTestBase.csName + "." + subclName2,
                ( BSONObject ) JSON
                        .parse( "{LowBound:{no:30000},UpBound:{no:40000}}" ) );
        return mainCL;
    }
}