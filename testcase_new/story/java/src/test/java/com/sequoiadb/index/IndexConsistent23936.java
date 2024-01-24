package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.Random;

/**
 * @Description seqDB-23936:主表并发创建和删除相同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23936 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23936";
    private String mainCLName = "maincl_23936";
    private String subCLName1 = "subcl_23936_1";
    private String subCLName2 = "subcl_23936_2";
    private String idxName = "index23936";
    private DBCollection maincl;
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
        int recordNum = 10000;
        IndexUtils.insertDataWithOutReturn( maincl, recordNum );
    }

    @Test
    public void test() throws Exception {
        // 并发创建和删除相同的索引
        ThreadExecutor te = new ThreadExecutor();
        CreateThread createIndex = new CreateThread();
        DropThread dropIndex = new DropThread();
        te.addWorker( createIndex );
        te.addWorker( dropIndex );
        te.run();

        Assert.assertEquals( createIndex.getRetCode(), 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                idxName, 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                idxName, 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                idxName, 0 );

        if ( dropIndex.getRetCode() == 0 ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName,
                    false );
            Assert.assertFalse( maincl.isIndexExist( idxName ) );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName1,
                    idxName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName2,
                    idxName, 0 );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                    idxName, 0 );
        } else if ( dropIndex.getRetCode() == SDBError.SDB_IXM_NOTEXIST
                .getErrorCode()
                || dropIndex.getRetCode() == SDBError.SDB_CLS_MUTEX_TASK_EXIST
                        .getErrorCode()
                || dropIndex.getRetCode() == SDBError.SDB_IXM_CREATING
                        .getErrorCode() ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName,
                    true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName,
                    true );
            Assert.assertTrue( maincl.isIndexExist( idxName ) );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Drop index",
                    csName, subCLName1 ) );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Drop index",
                    csName, subCLName2 ) );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Drop index",
                    csName, mainCLName ) );
        } else {
            Assert.fail( "dropIndex errorCode:" + dropIndex.getThrowable() );
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

    private class CreateThread extends ResultStore {
        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                cl.createIndex( idxName, new BasicBSONObject( "testno", 1 ),
                        false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class DropThread extends ResultStore {
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
