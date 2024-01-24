package com.sequoiadb.index;

import static com.sequoiadb.index.IndexUtil.assertIndexCreatedCorrect;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Vector;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * Created by laojingtang on 18-1-2.
 */
public class Index11414 extends SdbTestBase {
    final String CLNAME = "index11414";
    private Sequoiadb db = null;
    private DBCollection dbcl;
    private List< BSONObject > prepareData = new ArrayList<>( 10000 );

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( CLNAME );
    }

    private void prepareData() {
        // prepare data
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject().append( "a", i )
                    .append( "b", i ).append( "c", i ).append( "d", i );
            prepareData.add( obj );
        }
        dbcl.insert( prepareData );
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
     * 1、向cl中插入大量数据 2、创建索引时并发删除记录，分别覆盖两种删除场景： a、删除cl中所有记录 b、单条记录删除（单条删除集合中索引的记录）
     * 3、检查操作结果
     */
    @Test
    public void testCreateIndexAndRemoveAllRecord()
            throws InterruptedException {
        dbcl.delete( new BasicBSONObject() );
        prepareData.clear();
        prepareData();

        final IndexEntity index2Create = new IndexEntity()
                .setIndexName( "index11414" )
                .setKey( new BasicBSONObject( "a", 1 ) ).setUnique( false )
                .setEnforced( false );
        SdbThreadBase createIndexTasks = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11414.this.CLNAME );
                    cl.createIndex( index2Create.getIndexName(),
                            index2Create.getKey(), index2Create.isUnique(),
                            index2Create.isEnforced() );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase removeAllRecordTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11414.this.CLNAME );
                    for ( int i = 0; i < 1000; i++ ) {
                        cl.delete( new BasicBSONObject() );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        removeAllRecordTask.start( 20 );
        Thread.sleep( 100 + new Random().nextInt( 200 ) );
        createIndexTasks.start();

        assertTrue( removeAllRecordTask.isSuccess(),
                removeAllRecordTask.getErrorMsg() );
        assertTrue( createIndexTasks.isSuccess(),
                createIndexTasks.getErrorMsg() );

        // assert index created correctly
        assertIndexCreatedCorrect( dbcl, index2Create );

        // assert remove task already remove all recored.
        assertEquals( dbcl.getCount(), 0 );
    }

    @Test
    public void testCreateIndexAndRemoveRecords() {
        dbcl.delete( new BasicBSONObject() );
        prepareData.clear();
        prepareData();

        final IndexEntity indexEntity2Create = new IndexEntity()
                .setIndexName( "b_index" )
                .setKey( new BasicBSONObject( "b", 1 ) ).setEnforced( false )
                .setUnique( false );
        SdbThreadBase createIndexTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11414.this.CLNAME );
                    cl.createIndex( indexEntity2Create.getIndexName(),
                            indexEntity2Create.getKey(),
                            indexEntity2Create.isUnique(),
                            indexEntity2Create.isEnforced() );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        final List< ObjectId > oidRemoved = new Vector<>();
        SdbThreadBase removeRecordTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11414.this.CLNAME );
                    for ( int i = 0; i < 100; i++ ) {
                        BasicBSONObject obj = ( BasicBSONObject ) cl.queryOne();
                        ObjectId id = obj.getObjectId( "_id" );
                        cl.delete( new BasicBSONObject( "_id", id ) );
                        oidRemoved.add( id );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };
        createIndexTask.start();
        removeRecordTask.start( 20 );

        assertTrue( createIndexTask.isSuccess(),
                createIndexTask.getErrorMsg() );
        assertTrue( removeRecordTask.isSuccess(),
                removeRecordTask.getErrorMsg() );

        // assert index aleardy created.
        assertIndexCreatedCorrect( dbcl, indexEntity2Create );

        // assert remove task do right things.
        for ( ObjectId objectId : oidRemoved ) {
            DBCursor c = dbcl.query( new BasicBSONObject( "_id", objectId ),
                    new BasicBSONObject(), new BasicBSONObject(),
                    new BasicBSONObject() );
            assertFalse( c.hasNext(), objectId.toString() );
        }

        // assert index can be used.
        Map< Object, BSONObject > map = new Hashtable<>();
        for ( BSONObject obj : prepareData ) {
            map.put( obj.get( "_id" ), obj );
        }
        for ( ObjectId objectId : oidRemoved ) {
            map.remove( objectId );
        }

        BSONObject matcher = new BasicBSONObject();
        BSONObject selector = new BasicBSONObject();
        BSONObject orderby = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject( "",
                indexEntity2Create.getIndexName() );

        try ( DBCursor c = dbcl.query( matcher, selector, orderby, hint )) {
            while ( c.hasNext() ) {
                BSONObject obj = c.getNext();
                assertEquals( obj, map.get( obj.get( "_id" ) ) );
            }
        }
    }
}
