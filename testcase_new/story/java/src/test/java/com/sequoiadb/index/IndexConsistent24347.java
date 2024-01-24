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
 * @Description seqDB-24347:并发主表删除索引和子表切分
 * @Author liuli
 * @Date 2021.09.15
 * @UpdateAuthor liuli
 * @UpdateDate 2021.09.15
 * @version 1.10
 */
public class IndexConsistent24347 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_24347";
    private String mainCLName = "maincl_24347";
    private String subCLName1 = "subcl_24347_1";
    private String subCLName2 = "subcl_24347_2";
    private String subCLName3 = "subcl_24347_3";
    private String indexName1 = "index_24347_1";
    private String indexName2 = "index_24347_2";
    private String indexName3 = "index_24347_3";
    private ArrayList< String > groupNames = new ArrayList<>();
    private ArrayList< String > indexNames = new ArrayList<>();
    private boolean runSuccess = false;

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
        maincl.createIndex( indexName1, new BasicBSONObject( "testa", 1 ),
                null );
        maincl.createIndex( indexName2, new BasicBSONObject( "testb", 1 ),
                null );
        maincl.createIndex( indexName3, new BasicBSONObject( "testno", 1 ),
                null );
        subcl3.split( groupNames.get( 0 ), groupNames.get( 1 ), 50 );
        indexNames.add( indexName1 );
        indexNames.add( indexName2 );
        indexNames.add( indexName3 );
    }

    @Test
    public void test() throws Exception {
        // 并发主表删除索引，子表切分
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropIndex() );
        es.addWorker( new Split1() );
        es.addWorker( new Split2() );
        es.addWorker( new Split3() );
        es.run();

        // 校验索引一致性
        DBCollection maincl = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        for ( String indexName : indexNames ) {
            Assert.assertFalse( maincl.isIndexExist( indexName ) );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName3, indexName,
                    false );
        }
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
        private void threadrun() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                // 随机等待1500ms后再删除索引
                Random random = new Random();
                Thread.sleep( random.nextInt( 1500 ) );
                for ( String indexName : indexNames ) {
                    cl.dropIndex( indexName );
                }
            }
        }
    }

    // 切分subcl1，切分到多个组
    private class Split1 extends ResultStore {
        @ExecuteOrder(step = 1)
        private void threadrun() {
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
        private void threadrun() {
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
        private void threadrun() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subCLName3 );
                subcl.split( groupNames.get( 0 ), groupNames.get( 1 ), 100 );
            }
        }
    }
}
