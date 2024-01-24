package com.sequoiadb.index;

import static com.sequoiadb.index.IndexUtil.assertIndexCreatedCorrect;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.ConcurrentLinkedQueue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
public class Index11424 extends SdbTestBase {
    final String CLNAME = Index11424.class.getSimpleName();
    private Sequoiadb db = null;
    private DBCollection dbcl;
    private String[] clRowMeta = new String[] { "a0", "a1", "a2", "a3", "a4",
            "a5", "a6", "a7", "a8", "a9" };
    List< BSONObject > bsonInserted = new ArrayList<>( 1000 );

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dbcl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( CLNAME );
    }

    private void prepareData() {
        // prepare data
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            for ( String s : clRowMeta ) {
                obj.put( s, i );
            }
            bsonInserted.add( obj );
        }
        dbcl.insert( bsonInserted );
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
     * 1.插入1000条记录，其中每条记录存在10个字段 2.10个并发创建索引，分别为每个字段创建1个索引 3.10个并发带条件查询
     * 4.步骤2和步骤3并发执行，检查查询结果
     */
    @Test
    public void testCreateIndexAndRemoveAllRecord() {
        prepareData();
        final Random random = new Random();

        final ConcurrentLinkedQueue< IndexEntity > queue = new ConcurrentLinkedQueue<>();
        for ( String s : clRowMeta ) {
            queue.add( new IndexEntity().setIndexName( s + "_index" )
                    .setKey( new BasicBSONObject( s, 1 ) ).setEnforced( false )
                    .setUnique( false ) );
        }

        final IndexEntity[] indexArr = new IndexEntity[ queue.size() ];
        queue.toArray( indexArr );

        SdbThreadBase createIndexTasks = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11424.this.CLNAME );
                    IndexEntity index = queue.poll();
                    cl.createIndex( index.getIndexName(), index.getKey(),
                            index.isUnique(), index.isEnforced() );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase queryTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( Index11424.this.CLNAME );
                    long num = cl.getCount();

                    for ( int i = 0; i < 1000; i++ ) {
                        for ( String s : clRowMeta ) {
                            int sValue = random.nextInt( ( int ) num );
                            BasicBSONObject matcher = new BasicBSONObject( s,
                                    sValue );
                            DBCursor cursor = cl.query( matcher,
                                    new BasicBSONObject(),
                                    new BasicBSONObject(),
                                    new BasicBSONObject(), 0, 10 );
                            BSONObject o = cursor.getNext();
                            cursor.close();
                            assertEquals( o.get( s ), sValue, o.toString() );
                        }
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        createIndexTasks.start( clRowMeta.length );
        queryTask.start( 10 );

        assertTrue( queryTask.isSuccess(), queryTask.getErrorMsg() );
        assertTrue( createIndexTasks.isSuccess(),
                createIndexTasks.getErrorMsg() );

        // assert index already created.
        for ( IndexEntity expect : indexArr ) {
            assertIndexCreatedCorrect( dbcl, expect );
        }

        // assert index can use
        Map< Object, BSONObject > expectRecordMap = new HashMap<>();
        for ( BSONObject object : bsonInserted ) {
            expectRecordMap.put( object.get( "_id" ), object );
        }
        for ( IndexEntity expectIndex : indexArr ) {
            BSONObject matcher = new BasicBSONObject();
            BSONObject selector = new BasicBSONObject();
            BSONObject orderby = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject( "",
                    expectIndex.getIndexName() );

            try ( DBCursor cursor = dbcl.query( matcher, selector, orderby,
                    hint )) {
                while ( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    assertEquals( obj,
                            expectRecordMap.get( obj.get( "_id" ) ) );
                }
            }
        }
    }
}
