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
 * @Description seqDB-23952:并发删除索引和删除主表所在cs
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23952 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23952";
    private String mainCLName = "maincl_23952";
    private String subCLName1 = "subcl_23952_1";
    private String subCLName2 = "subcl_23952_2";
    private String idxName = "idx23952";
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
        DBCollection maincl = cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
        int recordNum = 20000;
        IndexUtils.insertDataWithOutReturn( maincl, recordNum );
        maincl.createIndex( idxName, "{testa:1}", false,false );
    }

    @Test
    public void test() throws Exception {
        // 并发主表删除索引和删除主表所在的CS
        ThreadExecutor es = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        DropCS dropCS = new DropCS();
        es.addWorker( dropIndex );
        es.addWorker( dropCS );
        es.run();

        // 校验结果
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, mainCLName );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName1 );
        IndexUtils.checkNoTask( sdb, "Drop index", csName, subCLName2 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropIndex() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                Random random = new Random();
                Thread.sleep( random.nextInt( 1500 ) );
                cl.dropIndex( idxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            }
        }
    }
}
