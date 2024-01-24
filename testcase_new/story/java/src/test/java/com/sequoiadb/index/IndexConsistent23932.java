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
 * @Description seqDB-23932:分区表并发删除不同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23932 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private String csName = "cs_23932";
    private String clName = "cl_23932";
    private DBCollection cl;
    private List< BSONObject > insertRecord = new ArrayList< BSONObject >();

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
            obj.put( "no", i );
            for ( int j = 0; j < 10; j++ ) {
                obj.put( "a" + j, i );
            }
            insertRecord.add( obj );
        }
        cl.insert( insertRecord );
    }

    @Test
    public void test() throws Exception {
        List< String > indexNames = new ArrayList<>();
        int indexNum = 10;
        int[] resultCode = { 0, SDBError.SDB_IXM_REDEF.getErrorCode() };
        // 创建10个索引
        for ( int i = 0; i < indexNum; i++ ) {
            String indexName = "index23932_" + i;
            indexNames.add( indexName );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
            cl.createIndex( indexName, new BasicBSONObject( "a" + i, 1 ), false,
                    false );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    true );
            // SEQUOIADBMAINSTREAM-7518，group中允许报错-247
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, resultCode );
        }

        // 并发删除10个索引
        ThreadExecutor es = new ThreadExecutor();
        for ( String indexName : indexNames ) {
            es.addWorker( new DropIndex( indexName ) );
        }
        es.run();

        // 检查主备一致性
        for ( String indexName : indexNames ) {
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
            IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName,
                    indexName, 0 );
        }

        // 校验数据，随机索引查询访问计划
        IndexUtils.checkRecords( cl, insertRecord, "", "" );
        Random random = new Random();
        int i = random.nextInt( indexNum );
        String matcher = "{'a" + i + "':" + i + "}";
        IndexUtils.checkExplain( cl, matcher, "tbscan", "" );
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
                        .getCollection( clName );
                cl.dropIndex( indexName );
            }
        }
    }
}
