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
 * @Description seqDB-23949:并发删除索引和删除主表
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23949 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23949";
    private String mainCLName = "maincl_23949";
    private String subCLName1 = "subcl_23949a";
    private String subCLName2 = "subcl_23949b";
    private String idxName = "idx23949";
    private CollectionSpace cs;
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

        cs = sdb.createCollectionSpace( csName );
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
        // 并发删除索引和删除主表
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        DropMaincl dropMaincl = new DropMaincl();
        es.addWorker( dropIndex );
        es.addWorker( dropMaincl );
        es.run();

        Assert.assertFalse( cs.isCollectionExist( mainCLName ) );
        Assert.assertFalse( cs.isCollectionExist( subCLName1 ) );
        Assert.assertFalse( cs.isCollectionExist( subCLName2 ) );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, mainCLName );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName1 );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName2 );
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
        private Sequoiadb db = null;
        private DBCollection cl = null;

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( mainCLName );
        }

        @ExecuteOrder(step = 2)
        private void dropIndex() {
            try {
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropMaincl extends ResultStore {
        @ExecuteOrder(step = 2)
        private void dropMaincl() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.dropCollection( mainCLName );
            }
        }
    }
}
