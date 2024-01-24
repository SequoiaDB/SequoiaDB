package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.testcommon.*;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;

/**
 * @Description seqDB-23931:主表并发删除相同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23931 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23931";
    private String mainCLName = "maincl_23931";
    private String subCLName1 = "subcl_23931_1";
    private String subCLName2 = "subcl_23931_2";
    private String idxName = "index23931";
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

        maincl.createIndex( idxName, new BasicBSONObject( "testno", 1 ), false,
                false );
        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( maincl, recordNum );
    }

    @Test
    public void test() throws Exception {

        // 并发删除
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex1 = new DropIndex();
        DropIndex dropIndex2 = new DropIndex();
        es.addWorker( dropIndex1 );
        es.addWorker( dropIndex2 );
        es.run();

        // 删除一个成功一个失败
        if ( dropIndex1.getRetCode() == SDBError.SDB_IXM_DROPPING.getErrorCode()
                || dropIndex1.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode() ) {
            Assert.assertEquals( dropIndex2.getRetCode(), 0,
                    "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                            + " dropIndex2 errorCode:"
                            + dropIndex2.getThrowable() );
        } else if ( dropIndex2.getRetCode() == SDBError.SDB_IXM_DROPPING
                .getErrorCode()
                || dropIndex2.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode() ) {
            Assert.assertEquals( dropIndex1.getRetCode(), 0,
                    "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                            + " dropIndex2 errorCode:"
                            + dropIndex2.getThrowable() );
        } else {
            Assert.fail( "dropIndex1 errorCode:" + dropIndex1.getThrowable()
                    + " dropIndex2 errorCode:" + dropIndex2.getThrowable() );
        }

        // 校验主备节点索引
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName,
                false );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName,
                false );
        Assert.assertFalse( maincl.isIndexExist( idxName ) );

        // 校验任务
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                idxName, 0 );
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName1,
                idxName, 0 );
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName2,
                idxName, 0 );
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

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}
