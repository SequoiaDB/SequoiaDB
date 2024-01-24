package com.sequoiadb.index;

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

import java.util.ArrayList;

/**
 * @Description seqDB-26360:主表并发复制和删除相同的索引
 * @Author liuli
 * @Date 2022.04.11
 * @UpdateAuthor liuli
 * @UpdateDate 2022.04.11
 * @version 1.10
 */
public class IndexConsistent26360 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_26360";
    private String mainCLName = "maincl_26360";
    private String subCLName1 = "subcl_26360_1";
    private String subCLName2 = "subcl_26360_2";
    private String indexName = "index26360";
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
        maincl.createIndex( indexName, new BasicBSONObject( "testno", 1 ),
                false, false );

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
        // 并发复制和删除相同的索引
        ThreadExecutor te = new ThreadExecutor();
        CopyIndex copyIndex = new CopyIndex();
        DropIndex dropIndex = new DropIndex();
        te.addWorker( copyIndex );
        te.addWorker( dropIndex );
        te.run();

        ArrayList< String > subclNames = new ArrayList<>();
        subclNames.add( csName + "." + subCLName1 );
        subclNames.add( csName + "." + subCLName2 );
        ArrayList< String > indexNames = new ArrayList<>();
        indexNames.add( indexName );
        // 校验结果
        if ( copyIndex.getRetCode() != 0 ) {
            // 复制索引失败，删除索引成功，最终索引不存在
            if ( copyIndex.getRetCode() != SDBError.SDB_IXM_NOTEXIST
                    .getErrorCode()
                    && copyIndex.getRetCode() != SDBError.SDB_IXM_DROPPING
                            .getErrorCode() ) {
                Assert.fail( "not expected error, copyIndex.getRetCode : "
                        + copyIndex.getRetCode() );
            }
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            // 校验任务和索引
            int[] resultCodes = { SDBError.SDB_IXM_NOTEXIST.getErrorCode(),
                    SDBError.SDB_IXM_DROPPING.getErrorCode() };
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                    indexName, 0 );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                    false );
        } else if ( dropIndex.getRetCode() != 0 ) {
            // 复制索引成功，删除索引失败
            if ( dropIndex.getRetCode() != SDBError.SDB_IXM_CREATING
                    .getErrorCode() ) {
                Assert.fail( "not expected error, dropIndex.getRetCode : "
                        + dropIndex.getRetCode() );
            }
            Assert.assertEquals( copyIndex.getRetCode(), 0 );
            // 校验任务和索引
            int[] resultCodes = { SDBError.SDB_IXM_CREATING.getErrorCode(),
                    SDBError.SDB_IXM_DROPPING.getErrorCode() };
            IndexUtils.checkCopyTask( sdb, csName, mainCLName, indexNames,
                    subclNames, 0, 9 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName, 0 );
            IndexUtils.checkNoTask( sdb, "Drop index", csName, mainCLName );
            IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName1 );
            IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName2 );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                    true );
        } else {
            // 先复制索引成功，再删除索引成功
            Assert.assertEquals( copyIndex.getRetCode(), 0 );
            Assert.assertEquals( dropIndex.getRetCode(), 0 );
            // 校验任务和索引
            IndexUtils.checkCopyTask( sdb, csName, mainCLName, indexNames,
                    subclNames, 0, 9 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName1,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName2,
                    indexName, 0 );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
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

    private class CopyIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void copyIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                maincl.copyIndex( null, indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                maincl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}