package com.sequoiadb.index.serial;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: CreateAndDropIndex10496.java test content:concurrent create
 * different indexs, than concurrent drop different indexes testlink
 * case:seqDB-10496
 * 
 * @author wuyan
 * @Date 2017.11.8
 * @version 1.00
 */
public class CreateAndDropIndex10496 extends SdbTestBase {
    final String CLNAME = "cl_10496";
    private Sequoiadb sdb = null;
    private DBCollection dbcl;

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( CLNAME );
        insertData( dbcl );
    }

    @Test
    private void CreateIndex() {
        String[] indexNames = { "testa", "testb", "testdecimal", "testchar",
                "testdate" };
        String[] indexKeys = { "{'a':1}", "{'b':-1}", "{'decimal':-1}",
                "{'str':1,'test':-1}", "{'date':1}" };
        //
        List< CreateIndexTask > createIndexTasks = new ArrayList<>( 5 );
        List< DropIndexTask > dropIndexTasks = new ArrayList<>( 5 );
        for ( int i = 0; i < 5; i++ ) {
            createIndexTasks.add(
                    new CreateIndexTask( indexNames[ i ], indexKeys[ i ] ) );
            dropIndexTasks.add( new DropIndexTask( indexNames[ i ] ) );
        }

        for ( CreateIndexTask createIndexTask : createIndexTasks ) {
            createIndexTask.start();
        }

        for ( CreateIndexTask createIndexTask : createIndexTasks ) {
            Assert.assertTrue( createIndexTask.isSuccess(),
                    createIndexTask.getErrorMsg() );
        }

        checkCreateIndexResult( indexNames, indexKeys );

        // concurrent drop different index
        for ( DropIndexTask dropIndexTask : dropIndexTasks ) {
            dropIndexTask.start();
        }

        for ( DropIndexTask dropIndexTask : dropIndexTasks ) {
            Assert.assertTrue( dropIndexTask.isSuccess(),
                    dropIndexTask.getErrorMsg() );
        }

        checkDropIndexResult( indexNames );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    private void teardown() {
        sdb.getCollectionSpace( SdbTestBase.csName ).dropCollection( CLNAME );
        sdb.disconnect();
    }

    private class CreateIndexTask extends SdbThreadBase {
        private String indexName;
        private String indexKey;

        public CreateIndexTask( String indexName, String indexKey ) {
            this.indexName = indexName;
            this.indexKey = indexKey;
        }

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                cl.createIndex( indexName,
                        ( BSONObject ) JSON.parse( indexKey ), false, false );
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }
    }

    private class DropIndexTask extends SdbThreadBase {
        private String indexName;

        public DropIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CLNAME );
                cl.dropIndex( indexName );
            } finally {
                if ( db != null )
                    db.disconnect();
            }
        }
    }

    private void insertData( DBCollection cl ) {
        for ( int i = 0; i < 100000; i += 10000 ) {
            List< BSONObject > list = new ArrayList< >();
            for ( int j = i + 0; j < i + 10000; j++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", i );
                obj.put( "b", i );
                obj.put( "c", i );
                obj.put( "test", "testeaaaaasdgadgaasdga" + i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                // insert the decimal type data
                String str = "32345.067891234567890123456789" + i;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimal", decimal );
                // the data type
                Date now = new Date();
                obj.put( "date", now );
                list.add( obj );
            }
            cl.insert( list );
        }
    }

    @SuppressWarnings("deprecation")
    private void checkCreateIndexResult( String[] indexNames,
            String[] indexKeys ) {
        for ( int i = 0; i < indexNames.length; i++ ) {
            DBCursor cursorIndex = dbcl.getIndex( indexNames[ i ] );
            while ( cursorIndex.hasNext() ) {
                // check the index info
                BSONObject object = cursorIndex.getNext();
                BSONObject record = ( BSONObject ) object.get( "IndexDef" );
                boolean actualUnique = ( boolean ) record.get( "unique" );
                Assert.assertEquals( actualUnique, false );

                boolean actualEnforced = ( boolean ) record.get( "enforced" );
                Assert.assertEquals( actualEnforced, false );
                Assert.assertEquals( record.get( "key" ),
                        JSON.parse( indexKeys[ i ] ) );
                // check the index status
                String indexFlag = ( String ) object.get( "IndexFlag" );
                Assert.assertEquals( indexFlag, "Normal" );
            }
            cursorIndex.close();
        }

    }

    @SuppressWarnings("deprecation")
    private void checkDropIndexResult( String[] indexNames ) {
        for ( int i = 0; i < indexNames.length; i++ ) {
            DBCursor cursorIndex = dbcl.getIndex( indexNames[ i ] );
            while ( cursorIndex.hasNext() ) {
                // check the index not exist
                BSONObject object = cursorIndex.getNext();
                Assert.assertNull( object,
                        "the index:" + indexNames[ i ] + " is exist!" );
            }
            cursorIndex.close();
        }

    }
}
