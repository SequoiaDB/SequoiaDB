package com.sequoiadb.index.serial;

import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23945:并发创建索引和renamecs
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23945 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private String csName = "cs_23945";
    private String newCSName = "newcs_23945";
    private String clName = "cl_23945";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String indexName = "testindex23945";
        CreateIndex createIndex = new CreateIndex( indexName );
        RenameCS renameCS = new RenameCS();
        es.addWorker( createIndex );
        es.addWorker( renameCS );

        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            if ( createIndex.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && createIndex.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()) {
                Assert.fail( "---createIndex fail! the error code = "
                        + createIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, clName );
            reCreateIndexAndCheckResult( sdb, newCSName, clName, indexName );
        } else if ( renameCS.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
            IndexUtils.checkRecords( cl, insertRecords, "",
                    "{'':'" + indexName + "'}" );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( renameCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, clName );
            IndexUtils.checkIndexTask( sdb, "Create index", newCSName, clName,
                    indexName );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) )
                    sdb.dropCollectionSpace( csName );
                if ( sdb.isCollectionSpaceExist( newCSName ) )
                    sdb.dropCollectionSpace( newCSName );
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
                        .getCollection( clName );
                cl.createIndex( indexName, "{testa:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class RenameCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待1s内时间再rename
                int waitTime = new Random().nextInt( 1000 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }

                db.renameCollectionSpace( csName, newCSName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void reCreateIndexAndCheckResult( Sequoiadb db, String csName,
            String clName, String indexName ) throws Exception {
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.createIndex( indexName, "{testa:1}", false, false );
        IndexUtils.checkIndexTaskResult( db, "Create index", csName, clName,
                indexName, 0 );
        IndexUtils.checkIndexConsistent( db, csName, clName, indexName, true );

        IndexUtils.checkRecords( dbcl, insertRecords, "",
                "{'':'" + indexName + "'}" );
    }

}