package com.sequoiadb.transaction;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description: seqDB-13632:事务中snapshot9 10
 * @Author laojingtang
 * @Date 2017-11-27
 */

public class TestTransaction13632 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl13632";
    private String commCSName;

    @BeforeClass
    public void setUp() {
        commCSName = SdbTestBase.csName;
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
            try {
                cs = sdb.createCollectionSpace( commCSName );
            } catch ( BaseException e ) {
                Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
            }
        } else {
            cs = sdb.getCollectionSpace( commCSName );
        }
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .createCollection( clName );
    }

    @Test
    public void testTransactionBeginCommit() {
        List< BSONObject > transactionList;
        sdb.beginTransaction();
        cl.insert( "{a:1}" );
        transactionList = getSnap( Sequoiadb.SDB_SNAP_TRANSACTIONS );
        Assert.assertTrue( transactionList.size() > 0,
                "actual: " + transactionList );
        transactionList = getSnap( Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT );
        Assert.assertTrue( transactionList.size() > 0,
                "actual: " + transactionList );
        sdb.commit();
    }

    private List< BSONObject > getSnap( int code ) {
        DBCursor cursor = sdb.getSnapshot( code, "", "", "" );
        List< BSONObject > transactionList = new ArrayList< BSONObject >( 5 );
        while ( cursor.hasNext() ) {
            BSONObject info = cursor.getNext();
            Assert.assertTrue( info.containsField( "TransactionIDSN" ) );
            transactionList.add( info );
        }
        return transactionList;
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( cl.getName() ) ) {
                cs.dropCollection( cl.getName() );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }
}
