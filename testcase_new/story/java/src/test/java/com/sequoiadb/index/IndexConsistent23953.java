package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
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

/**
 * @Description seqDB-23953:并发删除索引和删除子表所在cs
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23953 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23953";
    private String subCSName = "subcs_23953";
    private String mainCLName = "maincl_23953";
    private String subCLName1 = "subcl_23953_1";
    private String subCLName2 = "subcl_23953_2";
    private String idxName = "idx23953";
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
        if ( sdb.isCollectionSpaceExist( subCSName ) ) {
            sdb.dropCollectionSpace( subCSName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        CollectionSpace subcs = sdb.createCollectionSpace( subCSName );
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        DBCollection maincl = cs.createCollection( mainCLName, optionsM );

        subcs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( subCSName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
        int recordNum = 20000;
        IndexUtils.insertDataWithOutReturn( maincl, recordNum );
        maincl.createIndex( idxName, new BasicBSONObject( "testno", 1 ), null );
    }

    @Test
    public void test() throws Exception {
        // 并发删除索引和删除子表所在的CS
        ThreadExecutor te = new ThreadExecutor();
        DropIndex dropIndex = new DropIndex();
        DropSubCS dropSubCS = new DropSubCS();
        te.addWorker( dropIndex );
        te.addWorker( dropSubCS );
        te.run();

        Assert.assertFalse( sdb.isCollectionSpaceExist( subCSName ) );
        IndexUtils.checkNoTask( sdb, "Drop index", subCSName, subCLName1 );
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, mainCLName,
                idxName, 0 );
        IndexUtils.checkIndexTask( sdb, "Drop index", csName, subCLName2,
                idxName, 0 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( sdb.isCollectionSpaceExist( csName ) ) {
                    sdb.dropCollectionSpace( csName );
                }
                if ( sdb.isCollectionSpaceExist( subCSName ) ) {
                    sdb.dropCollectionSpace( subCSName );
                }
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
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropSubCS extends ResultStore {
        @ExecuteOrder(step = 2)
        private void dropSubCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( subCSName );
            }
        }
    }
}
