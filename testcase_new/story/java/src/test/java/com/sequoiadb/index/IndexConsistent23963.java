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
 * @description seqDB-23963:并发删除索引和truncate
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23963 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23963";
    private String indexName1 = "index23963a";
    private String indexName2 = "index23963b";
    private int recsNum = 50000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
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
        cl.createIndex( indexName1, "{no:1,testa:1}", true, false );
        cl.createIndex( indexName2, "{no:-1,testa:1}", true, false );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( 180000 );
        DropIndex dropIndex1 = new DropIndex( indexName1 );
        TruncateCL truncateCL = new TruncateCL();
        DropIndex dropIndex2 = new DropIndex( indexName2 );
        es.addWorker( dropIndex1 );
        es.addWorker( dropIndex2 );
        es.addWorker( truncateCL );

        es.run();

        // 任务有两种情况错误码：0成功 ，-321数据truncate中
        int succResultCode = 0;
        int failResultCode = -321;
        if ( dropIndex1.getRetCode() != 0 ) {
            Assert.assertEquals( dropIndex1.getRetCode(),
                    SDBError.SDB_DMS_TRUNCATED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    clName, indexName1, failResultCode );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName1, true );
        } else {
            Assert.assertEquals( dropIndex1.getRetCode(), 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    clName, indexName1, succResultCode );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName1, false );
        }
        if ( dropIndex2.getRetCode() != 0 ) {
            Assert.assertEquals( dropIndex2.getRetCode(),
                    SDBError.SDB_DMS_TRUNCATED.getErrorCode() );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    clName, indexName2, failResultCode );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName2, true );
        } else {
            Assert.assertEquals( dropIndex2.getRetCode(), 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                    clName, indexName2, succResultCode );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName2, false );
        }

        ArrayList< BSONObject > insertRecords = new ArrayList<>();
        IndexUtils.checkRecords( cl, insertRecords, "", "" );
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

    private class DropIndex extends ResultStore {
        private String indexName;

        private DropIndex( String indexName ) {

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
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_DMS_TRUNCATED
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