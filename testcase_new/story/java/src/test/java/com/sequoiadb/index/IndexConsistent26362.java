package com.sequoiadb.index;

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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-26362:并发创建索引和修改$shard索引
 * @Author liuli
 * @Date 2022.04.11
 * @UpdateAuthor liuli
 * @UpdateDate 2022.04.11
 * @version 1.10
 */
public class IndexConsistent26362 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_26362";
    private String clName = "cl_26362";
    private String indexName = "index26362";
    private boolean runSuc = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        DBCollection dbcl = cs.createCollection( clName, optionsM );

        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( dbcl, recordNum );
    }

    // SEQUOIADBMAINSTREAM-8311
    @Test(enabled = false)
    public void test() throws Exception {
        // 并发创建索引和alter $shard索引
        ThreadExecutor te = new ThreadExecutor();
        CreateIndex createIndex = new CreateIndex();
        AlterCollection alterCollection = new AlterCollection();
        te.addWorker( createIndex );
        te.addWorker( alterCollection );
        te.run();

        // 校验结果
        if ( createIndex.getRetCode() == 0 ) {
            // 创建索引失败，alter成功
            if ( alterCollection.getRetCode() != SDBError.SDB_IXM_COVER_CREATING
                    .getErrorCode() ) {
                Assert.fail( "not expected error, alterCollection.getRetCode : "
                        + alterCollection.getRetCode() );
            }

            // 校验任务和索引
            checkIndexKey( sdb, csName, clName, "$shard",
                    new BasicBSONObject( "no", 1 ) );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, 0 );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
        } else {
            // 创建索引成功，alter失败
            if ( createIndex.getRetCode() != SDBError.SDB_IXM_COVER_CREATING
                    .getErrorCode()
                    && createIndex
                            .getRetCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                    .getErrorCode() ) {
                Assert.fail( "not expected error, createIndex.getRetCode : "
                        + createIndex.getRetCode() );
            }
            Assert.assertEquals( alterCollection.getRetCode(), 0 );
            // 校验索引
            checkIndexKey( sdb, csName, clName, "$shard",
                    new BasicBSONObject( "testno", 1 ) );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
        }
        runSuc = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuc ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.createIndex( indexName, new BasicBSONObject( "testno", 1 ),
                        null, null );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class AlterCollection extends ResultStore {
        @ExecuteOrder(step = 1)
        private void aterCollection() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BasicBSONObject option = new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "testno", 1 ) );
                dbcl.alterCollection( option );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private void checkIndexKey( Sequoiadb sdb, String csName, String clName,
            String indexName, BasicBSONObject indexKey ) {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        BSONObject indexInfo = dbcl.getIndexInfo( indexName );
        BSONObject indexDef = ( BSONObject ) indexInfo.get( "IndexDef" );
        BasicBSONObject key = ( BasicBSONObject ) indexDef.get( "key" );
        Assert.assertEquals( key, indexKey );
    }

}