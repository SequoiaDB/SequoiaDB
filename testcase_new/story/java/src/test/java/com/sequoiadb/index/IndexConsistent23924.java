package com.sequoiadb.index;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @Description seqDB-23924:分区表并发创建不同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.26
 * @version 1.10
 */
public class IndexConsistent23924 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23924";
    private String clName = "cl_23924";
    private List< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private DBCollection cl;
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
        BasicBSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "hash" );
        option.put( "AutoSplit", true );
        cl = cs.createCollection( clName, option );
        for ( int i = 0; i < 100; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testb", i );
            obj.put( "no", i );
            for ( int j = 0; j < 10; j++ ) {
                obj.put( "a" + j, i );
            }
            insertRecords.add( obj );
        }
        cl.insert( insertRecords );
    }

    @Test
    public void test() throws Exception {
        // 并发创建10个索引
        ThreadExecutor es = new ThreadExecutor();
        List< String > indexNames = new ArrayList<>();
        int threadNum = 10;
        for ( int j = 0; j < threadNum; j++ ) {
            String indexName = "index23924_" + j;
            String indexKey = "a" + j;
            indexNames.add( indexName );
            es.addWorker( new CreateIndex( indexName, indexKey ) );
        }
        es.run();

        // 检查主备一致性,校验任务
        int[] resultCode = { 0, SDBError.SDB_IXM_REDEF.getErrorCode() };
        for ( String indexName : indexNames ) {
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, resultCode );
        }

        // 校验数据，随机查询访问计划
        IndexUtils.checkRecords( cl, insertRecords, "", "" );
        Random random = new Random();
        int i = random.nextInt( threadNum );
        String matcher = "{'a" + i + "':" + i + "}";
        IndexUtils.checkExplain( cl, matcher, "ixscan", indexNames.get( i ) );
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
                        .getCollection( clName );
                cl.createIndex( indexName, new BasicBSONObject( indexKey, 1 ),
                        false, false );
            }
        }
    }
}
