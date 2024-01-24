package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-24346:并发主表创建索引和子表切分
 * @Author liuli
 * @Date 2021.09.15
 * @UpdateAuthor liuli
 * @UpdateDate 2021.09.15
 * @version 1.10
 */
public class IndexConsistent24346 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_24346";
    private String mainCLName = "maincl_24346";
    private String subCLName1 = "subcl_24346_1";
    private String subCLName2 = "subcl_24346_2";
    private String subCLName3 = "subcl_24346_3";
    private String indexName = "index_24346";
    private ArrayList< String > groupNames = new ArrayList<>();
    private boolean runSuc = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 3 ) {
            throw new SkipException( "group less than three" );
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

        cs.createCollection( subCLName1,
                new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "no", 1 ) ).append( "Group",
                                groupNames.get( 0 ) ) );
        cs.createCollection( subCLName2,
                new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "no", 1 ) ).append( "Group",
                                groupNames.get( 0 ) ) );
        DBCollection subcl3 = cs.createCollection( subCLName3,
                new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "no", 1 ) ).append( "Group",
                                groupNames.get( 0 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );

        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "LowBound", new BasicBSONObject( "no", 20000 ) );
        option3.put( "UpBound", new BasicBSONObject( "no", 30000 ) );
        maincl.attachCollection( csName + "." + subCLName3, option3 );

        IndexUtils.insertDataWithOutReturn( maincl, 30000, 5 );
        subcl3.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
    }

    // SEQUOIADBMAINSTREAM-7785 报-116的问题未解决
    // SEQUOIADBMAINSTREAM-7856
    @Test(enabled = false)
    public void test() throws Exception {
        // 并发主表创建索引，子表切分
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex() );
        es.addWorker( new Split1() );
        es.addWorker( new Split2() );
        es.addWorker( new Split3() );
        es.run();

        // 校验索引一致性
        DBCollection maincl = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        Assert.assertTrue( maincl.isIndexExist( indexName ) );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName3, indexName,
                true );
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
        private void createIndex() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                // 随机等待1500ms再创建索引
                Random random = new Random();
                Thread.sleep( random.nextInt( 1500 ) );
                cl.createIndex( indexName, new BasicBSONObject( "testa", 1 ),
                        null );
            }
        }
    }

    // 切分subcl1，切分到多个组
    private class Split1 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void split() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName1 );
                subcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
                subcl.split( groupNames.get( 0 ), groupNames.get( 2 ), 30 );
            }
        }
    }

    // 切分subcl2，100%切分
    private class Split2 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void split() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName2 );
                subcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 100 );
            }
        }
    }

    // 切分subcl3，cl在group1和group2，将group1的数据100%切分到group2
    private class Split3 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void split() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName3 );
                subcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 100 );
            }
        }
    }
}
