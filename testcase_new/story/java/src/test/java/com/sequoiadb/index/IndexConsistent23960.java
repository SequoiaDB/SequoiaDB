package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.threadexecutor.ResultStore;
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

/**
 * @Description seqDB-23960:并发创建索引和插入数据
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23960 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23960";
    private String clName = "cl_23960";
    private String indexName1 = "index_23960_1";
    private String indexName2 = "index_23960_2";
    private String indexName3 = "index_23960_3";
    private String indexName4 = "index_23960_4";
    private DBCollection cl;
    private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private List< String > indexNames = new ArrayList<>();
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
        cl = cs.createCollection( clName );
        indexNames.add( indexName1 );
        indexNames.add( indexName2 );
        indexNames.add( indexName3 );
        indexNames.add( indexName4 );
    }

    @Test
    public void test() throws Exception {
        // 并发插入和创建索引
        ThreadExecutor te = new ThreadExecutor();
        int insertThreadNum = 30;
        for ( int i = 0; i < insertThreadNum; i++ ) {
            te.addWorker( new InsertData( i ) );
        }
        te.addWorker( new CreateIndex() );
        te.run();
        for ( int i = 0; i < 30000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "_id", i );
            obj.put( "no", i );
            obj.put( "a", i );
            obj.put( "b", i );
            insertRecords.add( obj );
        }

        for ( String indexName : indexNames ) {
            IndexUtils.checkIndexTask( sdb, "Create index", csName, clName,
                    indexName, 0 );
            IndexUtils.checkRecords( cl, insertRecords, "", "" );
        }

        String matcher = "{'no':5}";
        IndexUtils.checkExplain( cl, matcher, "ixscan", indexName1 );
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
        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.createIndex( indexName1, new BasicBSONObject( "no", 1 ),
                        false, false );
                dbcl.createIndex( indexName2, new BasicBSONObject( "a", 1 ),
                        false, false );
                dbcl.createIndex( indexName3,
                        new BasicBSONObject( "no", 1 ).append( "a", 1 ), false,
                        false );
                dbcl.createIndex( indexName4, new BasicBSONObject( "b", 1 ),
                        false, false );
            }
        }
    }

    private class InsertData extends ResultStore {
        private int i;

        public InsertData( int i ) {
            this.i = i;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
                for ( int j = i * 1000; j < ( i + 1 ) * 1000; j++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "_id", j );
                    obj.put( "no", j );
                    obj.put( "a", j );
                    obj.put( "b", j );
                    allRecords.add( obj );
                }
                dbcl.insert( allRecords );
            }
        }
    }
}
