package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-24640:并发查询记录和删除索引
 * @Author liuli
 * @Date 2021.11.14
 * @UpdateAuthor liuli
 * @UpdateDate 2021.11.14
 * @version 1.10
 */
public class Index24640 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_24640";
    private String clName = "cl_24640";
    private String indexName = "index_24640_";
    private DBCollection dbcl;
    private List< BSONObject > insertRecords = new ArrayList<>();
    private List< String > indexNames = new ArrayList<>();
    private String[] indexKeys = { "a", "b", "c", "d", "e", "f" };
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
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        dbcl = dbcs.createCollection( clName );

        for ( int i = 0; i < indexKeys.length; i++ ) {
            dbcl.createIndex( indexName + i,
                    new BasicBSONObject( indexKeys[ i ], 1 ), null );
            indexNames.add( indexName + i );
        }

        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( indexKeys[ 0 ], i );
            obj.put( indexKeys[ 1 ], i );
            obj.put( indexKeys[ 2 ], i );
            obj.put( indexKeys[ 3 ], i );
            obj.put( indexKeys[ 4 ], i );
            obj.put( indexKeys[ 5 ], i );
            insertRecords.add( obj );
        }
        dbcl.insert( insertRecords );

    }

    @Test
    public void test() throws Exception {
        // 并发删除索引和查询数据
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropIndex( indexNames ) );
        es.addWorker( new Query() );
        es.run();

        for ( String indexName : indexNames ) {
            Assert.assertFalse( dbcl.isIndexExist( indexName ),
                    indexName + " expect not exist, actual exist" );
        }
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
        private List< String > indexNames;

        public DropIndex( List< String > indexNames ) {
            this.indexNames = indexNames;
        }

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( String indexName : indexNames ) {
                    cl.dropIndex( indexName );
                }
            }
        }
    }

    private class Query extends ResultStore {

        @ExecuteOrder(step = 1)
        private void query() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                String orderBy = "{'a':1}";
                String hint = "";
                for ( String indexKey : indexKeys ) {
                    try {
                        int count = 0;
                        hint = "{'':'" + indexKey + "'}";
                        DBCursor cursor = cl.query( null, null, orderBy, hint,
                                DBQuery.FLG_QUERY_PARALLED );
                        while ( cursor.hasNext() ) {
                            BSONObject record = cursor.getNext();
                            BSONObject expRecord = insertRecords.get( count++ );
                            Assert.assertEquals( record, expRecord );
                        }
                        cursor.close();
                        Assert.assertEquals( count, insertRecords.size() );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_IXM_UNEXPECTED_STATUS
                                .getErrorCode()
                                && e.getErrorCode() != SDBError.SDB_SYS
                                        .getErrorCode() ) {
                            throw e;
                        }
                    }
                }
            }
        }
    }

}
