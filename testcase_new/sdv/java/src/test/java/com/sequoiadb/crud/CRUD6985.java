package com.sequoiadb.crud;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.ConcurrentLinkedQueue;

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
public class CRUD6985 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = CRUD6985.class.getSimpleName();
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
     * 1、增删改查并发操作 2、检查操作后数据正确性
     */
    @Test
    public void test() {
        // int i;
        ClTask insertTask = new ClTask() {
            private List< BSONObject > oidInserted = new Vector<>( 10000 );
            private List< BSONObject > expect = new Vector<>( 10000 );

            @Override
            protected void doCurdHere( DBCollection cl ) {
                // System.out.println(i);
                List< BSONObject > insertData = generate( 1000 );
                expect.addAll( insertData );
                cl.insert( insertData );
                oidInserted.addAll( insertData );
            }

            @Override
            CURDResult getActualResult() {
                ResultImpl r = new ResultImpl();
                r.setResultBson( oidInserted );
                return r;
            }

            @Override
            CURDResult getExpectResult() {
                ResultImpl r = new ResultImpl();
                r.setResultBson( expect );
                return r;
            }
        };

        final List< BSONObject > deleteTaskEexpect = generate( 1000 );
        CRUD6985.this.dbcl.insert( deleteTaskEexpect );
        ClTask deleteTask = new ClTask() {
            @SuppressWarnings({ "unchecked", "rawtypes" })
            private ConcurrentLinkedQueue< BSONObject > removeQueue = new ConcurrentLinkedQueue(
                    deleteTaskEexpect );

            @Override
            protected void doCurdHere( DBCollection cl ) {
                BSONObject o;
                while ( ( o = removeQueue.poll() ) != null ) {
                    cl.delete( new BasicBSONObject( "_id", o.get( "_id" ) ) );
                }
            }

            @Override
            CURDResult getExpectResult() {
                ResultImpl r = new ResultImpl();
                r.setResultBson( deleteTaskEexpect );
                return r;
            }
        };

        ClTask queryTask = new ClTask() {
            @Override
            protected void doCurdHere( DBCollection cl ) {
                for ( int i = 0; i < 100; i++ ) {
                    cl.queryOne();
                }
            }
        };

        final List< BSONObject > updateTaskExpect = generate( 1000 );
        CRUD6985.this.dbcl.insert( updateTaskExpect );
        ClTask updateTask = new ClTask() {
            @SuppressWarnings({ "unchecked", "rawtypes" })
            ConcurrentLinkedQueue< BSONObject > updateQueue = new ConcurrentLinkedQueue(
                    updateTaskExpect );

            @Override
            protected void doCurdHere( DBCollection cl ) {
                BSONObject o;
                while ( ( o = updateQueue.poll() ) != null ) {
                    cl.update( new BasicBSONObject( "_id", o.get( "_id" ) ),
                            ( BSONObject ) JSON.parse( "{$inc:{a:1}}" ),
                            new BasicBSONObject() );
                }
            }

            @Override
            CURDResult getExpectResult() {
                for ( BSONObject object : updateTaskExpect ) {
                    int a = ( int ) object.get( "a" );
                    a++;
                    object.put( "a", a );
                }
                ResultImpl r = new ResultImpl();
                r.setResultBson( updateTaskExpect );
                return r;
            }
        };

        insertTask.start( 10 );
        deleteTask.start( 10 );
        queryTask.start( 10 );
        updateTask.start( 10 );

        assertTrue( insertTask.isSuccess(), insertTask.getErrorMsg() );
        assertTrue( deleteTask.isSuccess(), deleteTask.getErrorMsg() );
        assertTrue( queryTask.isSuccess(), queryTask.getErrorMsg() );
        assertTrue( updateTask.isSuccess(), updateTask.getErrorMsg() );

        Map< ObjectId, BSONObject > actualRecord = new HashMap<>();
        DBCursor cursor = dbcl.query();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actualRecord.put( ( ObjectId ) obj.get( "_id" ), obj );
        }

        // assert insert
        ResultImpl r = ( ResultImpl ) insertTask.getExpectResult();
        for ( BSONObject object : r.getResultBson() ) {
            assertTrue( actualRecord.containsKey( object.get( "_id" ) ),
                    object.get( "_id" ).toString() );
        }

        // assert delete
        r = ( ResultImpl ) deleteTask.getExpectResult();
        for ( BSONObject object : r.getResultBson() ) {
            assertFalse( actualRecord.containsKey( object.get( "_id" ) ),
                    object.get( "_id" ).toString() );
        }

        // assert update
        r = ( ResultImpl ) updateTask.getExpectResult();
        for ( BSONObject object : r.getResultBson() ) {
            assertEquals( actualRecord.get( object.get( "_id" ) ), object );
        }
    }

    private List< BSONObject > generate( int i ) {
        List< BSONObject > list = new ArrayList<>( i );
        for ( int j = 0; j < i; j++ ) {
            list.add( new BasicBSONObject().append( "a", i ).append( "b", i )
                    .append( "c", i ).append( "_id", new ObjectId() ) );
        }
        return list;
    }

    abstract class ClTask extends SdbThreadBase {

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                doCurdHere( cl );
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }

        protected abstract void doCurdHere( DBCollection cl );

        CURDResult getActualResult() {
            return new CURDResult() {
            };
        }

        CURDResult getExpectResult() {
            return new CURDResult() {
            };
        }
    }

    interface CURDResult {
    }

    class ResultImpl implements CURDResult {
        private List< BSONObject > resultBson;

        public void setResultBson( List< BSONObject > resultBson ) {
            this.resultBson = resultBson;
        }

        public List< BSONObject > getResultBson() {
            return resultBson;
        }
    }
}
