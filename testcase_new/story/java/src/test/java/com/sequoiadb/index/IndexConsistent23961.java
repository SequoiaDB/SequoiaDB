package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.threadexecutor.ResultStore;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23961:并发创建索引和truncate
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23961 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23961";
    private int recsNum = 50000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException(
                    "---Skip testCase.Current environment less than tow groups! " );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23961";
        ThreadExecutor es = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex( indexName );
        TruncateCL truncateCL = new TruncateCL();
        es.addWorker( createIndex );
        es.addWorker( truncateCL );
        es.run();

        // cl执行truncate后任务显示集合被清空-321；未truncate之前创建索引则resultCode为0
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertTrue( createIndex.getRetCode() == SDBError.SDB_DMS_TRUNCATED.getErrorCode() ||
                               createIndex.getRetCode() == SDBError.SDB_TASK_HAS_CANCELED.getErrorCode() );
            int[] resultCodes = { 0, -321, -247, -243 };
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, resultCodes );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
        } else {
            //SEQUOIADBMAINSTREAM-8140发生-321错误重试一次，主节点Group Result可能报错-247，最终索引创建成功
            int[] resultCodes = { 0, -247 };
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, resultCodes );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
        }

        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        IndexUtils.checkRecords( cl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
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
                        .getCollection( clName );
                cl.createIndex( indexName, "{no:1,testa:1}", true, false );
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_DMS_TRUNCATED
                        .getErrorType() &&
                     e.getErrorType() != SDBError.SDB_TASK_HAS_CANCELED
                        .getErrorType() ) {
                    throw e;
                }
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class TruncateCL {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {

                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.truncate();
            }
        }
    }
}
