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
 * @description seqDB-23943:并发创建索引和删除主表所在cs(主表和子表在相同cs)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23943 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection dbcl;
    private String csName = "cs_23943";
    private String mainclName = "maincl_23943";
    private String subclName1 = "subcl_23943a";
    private String subclName2 = "subcl_23943b";
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
        String indexName = "testindex23943aaaaaaaaaaa$%";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        DropCS dropCS = new DropCS();
        es.addWorker( createIndex );
        es.addWorker( dropCS );
        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_DELETING
                            .getErrorCode()
                    && createIndex
                            .getRetCode() != SDBError.SDB_DMS_SCANNER_INTERRUPT
                                    .getErrorCode()
                    && createIndex
                            .getRetCode() != SDBError.SDB_TASK_HAS_CANCELED
                                    .getErrorCode() ) {
                Assert.fail( "---errorCode=" + createIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, mainclName );
        } else if ( dropCS.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainclName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName1, indexName,
                    true );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                    true );
            IndexUtils.checkRecords( dbcl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, mainclName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) )
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

    private class DropCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待1S内时间再删除cl
                int waitTime = new Random().nextInt( 1000 );
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
