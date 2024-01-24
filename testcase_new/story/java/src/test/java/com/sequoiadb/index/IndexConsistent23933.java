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
 * @Description seqDB-23933:主表并发删除不同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23933 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23933";
    private String mainCLName = "maincl_23933";
    private String subCLName1 = "subcl_23933_1";
    private String subCLName2 = "subcl_23933_2";
    private DBCollection maincl;
    private List< BSONObject > insertRecord = new ArrayList< BSONObject >();
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

        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject();
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
        int indexNum = 10;
        // 创建10个索引
        for ( int i = 0; i < indexNum; i++ ) {
            String indexName = "index23933_" + i;
            indexNames.add( indexName );
            maincl.createIndex( indexName, new BasicBSONObject( "a" + i, 1 ),
                    false, false );
        }
        // 并发删除10个索引
        ThreadExecutor es = new ThreadExecutor();
        for ( String indexName : indexNames ) {
            es.addWorker( new DropIndex( indexName ) );
        }
        es.run();

        // 检查主备一致性
        for ( String indexName : indexNames ) {
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, indexName,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, indexName,
                    false );
            Assert.assertFalse( maincl.isIndexExist( indexName ) );
        }

        // 校验数据，随机索引查询访问计划
        IndexUtils.checkRecords( maincl, insertRecord, "", "" );
        Random random = new Random();
        int i = random.nextInt( indexNum );
        String matcher = "{'a" + i + "':" + i + "}";
        IndexUtils.checkExplain( maincl, matcher, "tbscan", "" );
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

    private class DropIndex {
        private String indexName;

        public DropIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                cl.dropIndex( indexName );
            }
        }
    }
}
