package com.sequoiadb.crud;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertTrue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Index6986 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Index6986.class.getSimpleName();
    private DBCollection dbcl;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( CLNAME );
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
     * 1、持续插入数据，过程中创建索引 2、检查数据和索引正确性 1、插入数据成功，查看CL中数据正确
     * 2、创建索引成功，查看各节点索引均存在且状态为nomal 3、查询索引字段，查询结果正确，且有走索引
     */
    @SuppressWarnings("deprecation")
    @Test
    public void test() throws InterruptedException {
        final List< BSONObject > insertData = new ArrayList<>( 10000 );
        SdbThreadBase insert = new SdbThreadBase() {

            @SuppressWarnings({ "resource" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    for ( int i = 0; i < 100; i++ ) {
                        List< BSONObject > t = generate( 500 );
                        cl.insert( t );
                        insertData.addAll( t );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase createIndex = new SdbThreadBase() {
            @SuppressWarnings({ "resource" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    cl.createIndex( "a_index", new BasicBSONObject( "a", 1 ),
                            false, false );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        insert.start();
        Thread.sleep( 300 + new Random().nextInt( 300 ) );
        createIndex.start();

        assertTrue( insert.isSuccess(), insert.getErrorMsg() );
        assertTrue( createIndex.isSuccess(), createIndex.getErrorMsg() );

        Map< ObjectId, BSONObject > actualRecord = new HashMap<>();
        DBCursor cursor = dbcl.query();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actualRecord.put( ( ObjectId ) obj.get( "_id" ), obj );
        }

        // assert insert
        for ( BSONObject insertDatum : insertData ) {
            assertEquals( actualRecord.get( insertDatum.get( "_id" ) ),
                    insertDatum );
        }

        // assert index aleardy created.
        DBCursor cur = dbcl.getIndex( "a_index" );
        BSONObject object = cur.getNext();
        assertNotNull( object );
        cur.close();
        BasicBSONObject indexDef = ( BasicBSONObject ) object.get( "IndexDef" );
        BasicBSONObject indexKey = ( BasicBSONObject ) indexDef.get( "key" );
        assertNotNull( object, "a_index" );
        assertEquals( indexDef.getString( "name" ), "a_index" );
        assertEquals( indexDef.getBoolean( "unique" ), false );
        assertEquals( indexDef.getBoolean( "enforced" ), false );
        assertEquals( indexKey, JSON.parse( "{a:1}" ) );
        assertEquals( object.get( "IndexFlag" ), "Normal" );
    }

    private List< BSONObject > generate( int i ) {
        List< BSONObject > list = new ArrayList<>( i );
        for ( int j = 0; j < i; j++ ) {
            list.add( new BasicBSONObject().append( "a", i ).append( "b", i )
                    .append( "c", i ).append( "_id", new ObjectId() ) );
        }
        return list;
    }
}
