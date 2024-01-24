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
 * @Description seqDB-26361:主表并发创建相同索引（索引定义或名称相同）
 * @Author liuli
 * @Date 2022.04.11
 * @UpdateAuthor liuli
 * @UpdateDate 2022.04.11
 * @version 1.10
 */
public class IndexConsistent26361 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_26361";
    private String mainCLName = "maincl_26361";
    private String subCLName1 = "subcl_26361_1";
    private String subCLName2 = "subcl_26361_2";
    private String indexName1 = "index26361_1";
    private String indexName2 = "index26361_2";
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
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        DBCollection maincl = cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 3000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 3000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
        int recordNum = 20000;
        IndexUtils.insertDataWithOutReturn( maincl, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 并发创建相同的索引
        ThreadExecutor te = new ThreadExecutor();
        // createIndex1创建索引(indexName1,{"testno":1})
        CreateIndex createIndex1 = new CreateIndex( indexName1,
                new BasicBSONObject( "testno", 1 ) );
        // createIndex2创建索引(indexName2,{"testno":1})，与createIndex1索引定义相同，名称不同
        CreateIndex createIndex2 = new CreateIndex( indexName2,
                new BasicBSONObject( "testno", 1 ) );
        // createIndex3创建索引(indexName1,{"testb":1})，与createIndex1索引定义不同，名称相同
        CreateIndex createIndex3 = new CreateIndex( indexName1,
                new BasicBSONObject( "testb", 1 ) );
        te.addWorker( createIndex1 );
        te.addWorker( createIndex2 );
        te.addWorker( createIndex3 );
        te.run();

        // 校验结果
        if ( createIndex1.getRetCode() == 0 ) {
            // 创建索引(indexName1,{"testno":1})成功，其他失败
            if ( createIndex2.getRetCode() != SDBError.SDB_IXM_COVER_CREATING
                    .getErrorCode()
                    && createIndex1
                            .getRetCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                    .getErrorCode() ) {
                Assert.fail( "not expected error, createIndex2.getRetCode : "
                        + createIndex2.getRetCode() );
            }
            if ( createIndex3
                    .getRetCode() != SDBError.SDB_IXM_SAME_NAME_CREATING
                            .getErrorCode()
                    && createIndex1.getRetCode() != SDBError.SDB_IXM_EXIST
                            .getErrorCode() ) {
                Assert.fail( "not expected error, createIndex3.getRetCode : "
                        + createIndex3.getRetCode() );
            }
            // 校验任务和索引
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                    indexName1, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName1, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName1, 0 );
            checkIndexKey( sdb, csName, mainCLName, indexName1,
                    new BasicBSONObject( "testno", 1 ) );
            checkIndexKey( sdb, csName, subCLName1, indexName1,
                    new BasicBSONObject( "testno", 1 ) );
            checkIndexKey( sdb, csName, subCLName2, indexName1,
                    new BasicBSONObject( "testno", 1 ) );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName1, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName1, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName2, false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName2, false );
        } else {
            // 创建索引(indexName1,{"testno":1})失败，其他成功
            if ( createIndex1
                    .getRetCode() != SDBError.SDB_IXM_SAME_NAME_CREATING
                            .getErrorCode()
                    && createIndex1
                            .getRetCode() != SDBError.SDB_IXM_COVER_CREATING
                                    .getErrorCode()
                    && createIndex1
                            .getRetCode() != SDBError.SDB_IXM_EXIST_COVERD_ONE
                                    .getErrorCode()
                    && createIndex1.getRetCode() != SDBError.SDB_IXM_EXIST
                            .getErrorCode() ) {
                Assert.fail( "not expected error, dropIndex.getRetCode : "
                        + createIndex1.getRetCode() );
            }
            Assert.assertEquals( createIndex2.getRetCode(), 0 );
            Assert.assertEquals( createIndex3.getRetCode(), 0 );
            // 校验任务和索引
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                    indexName1, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName1, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName1, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                    indexName2, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName2, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName2, 0 );
            checkIndexKey( sdb, csName, mainCLName, indexName1,
                    new BasicBSONObject( "testb", 1 ) );
            checkIndexKey( sdb, csName, subCLName1, indexName1,
                    new BasicBSONObject( "testb", 1 ) );
            checkIndexKey( sdb, csName, subCLName2, indexName1,
                    new BasicBSONObject( "testb", 1 ) );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName1, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName1, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1,
                    indexName2, true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2,
                    indexName2, true );
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
        private String indexName;
        private BasicBSONObject indexKey;

        private CreateIndex( String indexName, BasicBSONObject indexKey ) {
            this.indexName = indexName;
            this.indexKey = indexKey;
        }

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                maincl.createIndex( indexName, indexKey, null, null );
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