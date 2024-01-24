package com.sequoiadb.crud;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * Created by laojingtang on 18-1-3.
 */
public class Sdv8765 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection dbcl;
    private static final String CLNAME = Sdv8765.class.getSimpleName();

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

    private BSONObject[] genrateData( int num ) {
        BSONObject[] b = new BSONObject[ num ];
        for ( int i = 0; i < num; i++ ) {
            b[ i ] = new BasicBSONObject( "a", i );
        }
        return b;
    }

    /**
     * 2、创建cl，持续插入数据 3、插入数据过程中，并发执行创建/删除CL操作
     */
    @Test
    public void test() {
        SdbThreadBase insertData = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( CLNAME );
                    for ( int i = 0; i < 100; i++ ) {
                        cl.insert( Arrays.asList( genrateData( 1000 ) ) );
                    }
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        SdbThreadBase createAndDropCLTask = new SdbThreadBase() {
            @SuppressWarnings({ "resource", "deprecation" })
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    String clName = CLNAME + Thread.currentThread().getId();
                    CollectionSpace cs = db
                            .getCollectionSpace( SdbTestBase.csName );
                    cs.createCollection( clName );
                    cs.dropCollection( clName );
                } finally {
                    if ( db != null )
                        db.disconnect();
                }
            }
        };

        insertData.start();
        createAndDropCLTask.start( 20 );

        assertTrue( insertData.isSuccess(), insertData.getErrorMsg() );
        assertTrue( createAndDropCLTask.isSuccess(),
                createAndDropCLTask.getErrorMsg() );

        assertEquals( dbcl.getCount(), 100 * 1000 );
    }
}
