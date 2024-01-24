package com.sequoiadb.index;

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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23942:并发创建索引和删除cs
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23942 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private String csName = "cs_23942";
    private String clName = "cl_23942";
    private int recsNum = 20000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone." );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
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
        String indexName = "testindex23942";
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
                    && createIndex
                            .getRetCode() != SDBError.SDB_DMS_SCANNER_INTERRUPT
                                    .getErrorCode()
                    && createIndex
                            .getRetCode() != SDBError.SDB_TASK_HAS_CANCELED
                                    .getErrorCode() ) {
                Assert.fail( "---errorCode=" + createIndex.getRetCode() );
            }
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, clName );
        } else if ( dropCS.getRetCode() != 0 ) {
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(),
                    SDBError.SDB_LOCK_FAILED.getErrorCode() );
            IndexUtils.checkRecords( cl, insertRecords, "", "" );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
        } else {
            // 如果没有并发执行，则串行执行都成功
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            Assert.assertEquals( dropCS.getRetCode(), 0 );
            Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
            IndexUtils.checkNoTask( sdb, "Create index", csName, clName );
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
                        .getCollection( clName );
                cl.createIndex( indexName, "{testno:1}", false, false );
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
}
