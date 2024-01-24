package com.sequoiadb.sdb;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-19950:getList新增hint/limit/skip参数，并新增list类型
 * @Author huangxiaoni
 * @Date 2019.10.10
 */

public class GetListOrSnapshot19950 extends SdbTestBase {
    private int runSuccNum = 0;
    private int expRunSuccNum = 2;
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private int clNum = 3;
    private String clNameBase = "cl19950_";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone." );
        }
        cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( clNameBase + i );
        }
    }

    @Test
    public void test_getList() {
        // test hint / skip / limit
        BasicBSONObject queryObj = new BasicBSONObject();
        queryObj.put( "$regex", "^" + csName + "." + clNameBase );
        queryObj.put( "$options", "i" );
        BSONObject query = new BasicBSONObject( "Name", queryObj );
        BSONObject hint = new BasicBSONObject( "", "test" );
        long skipRows = 1;
        long returnRows = 1;
        DBCursor cursor = sdb.getList( Sequoiadb.SDB_LIST_COLLECTIONS, query,
                null, null, hint, skipRows, returnRows );
        int size = 0;
        while ( cursor.hasNext() ) {
            Object name = cursor.getNext().get( "Name" );
            // records disorder, the results are not unique
            Assert.assertTrue(
                    name.toString().contains( csName + "." + clNameBase ) );
            size++;
        }
        Assert.assertEquals( size, returnRows );

        // test listType: SDB_LIST_USERS
        cursor = sdb.getList( Sequoiadb.SDB_LIST_USERS, null, null, null );
        while ( cursor.hasNext() ) {
            Object user = cursor.getNext().get( "User" );
            Assert.assertNotNull( user );
        }

        runSuccNum++;
    }

    @Test
    public void test_getSnapshot() {
        // test param: hint / skip / limit
        BasicBSONObject queryObj = new BasicBSONObject();
        queryObj.put( "$regex", "^" + csName + "." + clNameBase );
        queryObj.put( "$options", "i" );
        BSONObject query = new BasicBSONObject( "Name", queryObj );
        BSONObject hint = new BasicBSONObject( "", "test" );
        long skipRows = 1;
        long returnRows = 1;
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONS,
                query, null, null, hint, skipRows, returnRows );
        int size = 0;
        while ( cursor.hasNext() ) {
            Object name = cursor.getNext().get( "Name" );
            // records disorder, the results are not unique
            Assert.assertTrue(
                    name.toString().contains( csName + "." + clNameBase ) );
            size++;
        }
        Assert.assertEquals( size, returnRows );

        // test snapType: SDB_SNAP_SVCTASKS
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SVCTASKS,
                new BasicBSONObject(), null, null );
        while ( cursor.hasNext() ) {
            Object taskName = cursor.getNext().get( "TaskName" );
            Assert.assertNotNull( taskName );
        }

        runSuccNum++;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccNum == expRunSuccNum ) {
                for ( int i = 0; i < clNum; i++ ) {
                    cs.dropCollection( clNameBase + i );
                }
            }
        } finally {
            sdb.disconnect();
        }
    }
}
