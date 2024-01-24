package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
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
import java.util.List;
import java.util.Random;

/**
 * @Description seqDB-23925:主表并发创建不同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23925 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23925";
    private String mainCLName = "maincl_23925";
    private String subCLName1 = "subcl_23925_1";
    private String subCLName2 = "subcl_23925_2";
    private List< BSONObject > insertRecord = new ArrayList< BSONObject >();
    private CollectionSpace dbcs;
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

        dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject optionM = new BasicBSONObject();
        optionM.put( "IsMainCL", true );
        optionM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionM.put( "ShardingType", "range" );
        maincl = dbcs.createCollection( mainCLName, optionM );

        dbcs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        dbcs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 5000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 5000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );

        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testb", i );
            obj.put( "no", i );
            for ( int j = 0; j < 10; j++ ) {
                obj.put( "a" + j, i );
            }
            insertRecord.add( obj );
        }
        maincl.insert( insertRecord );
    }

    @Test
    public void test() throws Exception {
        List< String > indexNames = new ArrayList<>();
        // 并发创建10个索引
        int threadNum = 10;
        ThreadExecutor es = new ThreadExecutor();
        for ( int j = 0; j < threadNum; j++ ) {
            String indexName = "index23925_" + j;
            String indexKey = "a" + j;
            indexNames.add( indexName );
            es.addWorker( new CreateIndex( indexName, indexKey ) );
        }
        es.run();

        // 检查主备一致性并校验任务
        for ( String indexName : indexNames ) {
            Assert.assertTrue( maincl.isIndexExist( indexName ) );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    true );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                    true );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, mainCLName,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName1,
                    indexName, 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subCLName2,
                    indexName, 0 );
        }

        // 校验数据，随机查询访问计划
        IndexUtils.checkRecords( maincl, insertRecord, "", "" );
        Random random = new Random();
        int i = random.nextInt( threadNum );
        String matcher = "{'a" + i + "':" + i + "}";
        IndexUtils.checkExplain( maincl, matcher, "ixscan",
                indexNames.get( i ) );
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

    private class CreateIndex {
        private String indexName;
        private String indexKey;

        public CreateIndex( String indexName, String indexKey ) {
            this.indexName = indexName;
            this.indexKey = indexKey;
        }

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                cl.createIndex( indexName, new BasicBSONObject( indexKey, 1 ),
                        false, false );
            }
        }
    }
}
