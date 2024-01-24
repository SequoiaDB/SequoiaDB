package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.ArrayList;

/**
 * @Description seqDB-23934:主表和子表并发删除相同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23934 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23934";
    private String mainCLName = "maincl_23934";
    private String subCLName1 = "subcl_23934_1";
    private String subCLName2 = "subcl_23934_2";
    private String idxName = "index23926";
    private DBCollection maincl;
    private boolean runSuccess = false;

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
        maincl = cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 5000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 5000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
    }

    @Test
    public void test() throws Exception {
        int recordNum = 10000;
        ArrayList< BSONObject > insertRecord = IndexUtils.insertData( maincl,
                recordNum );
        // 创建索引
        maincl.createIndex( idxName, new BasicBSONObject( "testno", 1 ), false,
                false );
        // 并发删除索引
        ThreadExecutor te = new ThreadExecutor();
        DropIndex mainDropIndex = new DropIndex( csName, mainCLName, idxName );
        DropIndex subDropIndex = new DropIndex( csName, subCLName1, idxName );
        te.addWorker( mainDropIndex );
        te.addWorker( subDropIndex );
        te.run();

        if ( subDropIndex.getRetCode() != SDBError.SDB_IXM_DROPPING
                .getErrorCode()
                && subDropIndex.getRetCode() != SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode()
                && subDropIndex.getRetCode() != 0 ) {
            Assert.fail(
                    "subDropIndex errorCode:" + subDropIndex.getThrowable() );
        }

        // 检查子表subCLName1索引和任务，子表subCLName1索引一定不存在
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName,
                false );
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName1,
                idxName, 0 );

        // 检查主表和子表subCLName2
        if ( mainDropIndex.getRetCode() == 0 ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName,
                    false );
            Assert.assertFalse( maincl.isIndexExist( idxName ) );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                    idxName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName2,
                    idxName, 0 );
        } else if ( mainDropIndex.getRetCode() == SDBError.SDB_IXM_DROPPING
                .getErrorCode() ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName,
                    true );
            Assert.assertTrue( maincl.isIndexExist( idxName ) );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "mainCLName",
                    csName, mainCLName ) );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "mainCLName",
                    csName, subCLName2 ) );
        } else {
            Assert.fail(
                    "mainDropIndex errorCode:" + mainDropIndex.getThrowable() );
        }

        IndexUtils.checkRecords( maincl, insertRecord, "", "" );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropIndex extends ResultStore {
        private String csName;
        private String clName;
        private String indexName;

        public DropIndex( String csName, String clName, String indexName ) {
            this.csName = csName;
            this.clName = clName;
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
