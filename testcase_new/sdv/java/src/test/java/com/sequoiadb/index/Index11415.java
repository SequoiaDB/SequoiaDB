package com.sequoiadb.index;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * Created by laojingtang on 18-1-2.
 */
public class Index11415 extends SdbTestBase {
    final String CLNAME = Index11415.class.getSimpleName();
    private Sequoiadb db = null;
    private DBCollection dbcl;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( CLNAME );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void teardown() {
        if ( db != null ) {
            db.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( CLNAME );
            db.disconnect();
        }
    }

    /**
     * 1、向cl中插入记录，过程中并发删除索引（插入记录中包含索引键） 2、检查操作结果
     */
    @SuppressWarnings("deprecation")
    @Test
    public void testRemoveIndex() {
        dbcl.createIndex( "a_index", new BasicBSONObject( "a", 1 ), false,
                false );
        dbcl.createIndex( "b_index", new BasicBSONObject( "b", 1 ), false,
                false );
        dbcl.createIndex( "c_index", new BasicBSONObject( "c", 1 ), false,
                false );
        dbcl.createIndex( "d_index", new BasicBSONObject( "d", 1 ), false,
                false );
        String[] indexNameArr = new String[] { "a_index", "b_index", "c_index",
                "d_index" };

        final ConcurrentLinkedQueue< String > indexQueue = new ConcurrentLinkedQueue<>();
        for ( String s : indexNameArr ) {
            indexQueue.add( s );
        }

        SdbThreadBase removeIndexTask = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11415.this.CLNAME );
                    String indexName = indexQueue.poll();
                    cl.dropIndex( indexName );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -47 )
                        throw e;
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };
        SdbThreadBase insertClTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11415.this.CLNAME );
                    // prepare data
                    List< BSONObject > list = new ArrayList<>( 10000 );
                    for ( int i = 0; i < 10000; i++ ) {
                        BSONObject obj = new BasicBSONObject().append( "a", i )
                                .append( "b", i ).append( "c", i )
                                .append( "d", i );
                        list.add( obj );
                    }
                    cl.insert( list );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };
        insertClTask.start( 10 );
        removeIndexTask.start( 4 );

        assertTrue( insertClTask.isSuccess(), insertClTask.getErrorMsg() );
        assertTrue( removeIndexTask.isSuccess(),
                removeIndexTask.getErrorMsg() );
        assertEquals( dbcl.getCount(), 10000 * 10 );

        for ( String s : indexNameArr ) {
            DBCursor curor = dbcl.getIndex( s );
            assertFalse( curor.hasNext(), s );
            curor.close();
        }
    }
}
