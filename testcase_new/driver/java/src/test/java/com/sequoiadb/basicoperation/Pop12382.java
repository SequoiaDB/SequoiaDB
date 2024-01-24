package com.sequoiadb.basicoperation;

import java.util.List;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @descreption seqDB-12382:pop记录
 * @author liuxiaoxuan
 * @date 2017.8.14
 * @updateUser YiPan
 * @updateDate 2021.12.3
 * @updateRemark 游标使用后立即关闭，优化部分语法
 * @version 1.0
 */
public class Pop12382 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection cappedCL = null;
    private String cappedCSName = "story_java_cappedCS_12382";
    private String cappedCLName = "cappedCL_12382";
    private List< BSONObject > insrtObjs = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        boolean isCapped = true;
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        cappedCL = CappedCLUtils.createCL( sdb, cappedCSName, cappedCLName,
                isCapped );
        int recordNums = 10;
        insertRecords( recordNums );
    }

    public void insertRecords( int recordNums ) {
        for ( int i = 0; i < recordNums; i++ ) {
            BSONObject obj = ( BSONObject ) JSON.parse( "{ a :" + i + "}" );
            cappedCL.insert( obj );
        }

        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );
        while ( cursor.hasNext() ) {
            insrtObjs.add( cursor.getNext() );
        }
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( cappedCSName );
            if ( cs != null && cs.isCollectionExist( cappedCLName ) ) {
                sdb.dropCollectionSpace( cappedCSName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    public void testPop() {
        boolean isHasDirection = true;
        Assert.assertTrue( popValidRecords( isHasDirection ),
                "pop valid records failed" );
        Assert.assertTrue( popValidRecords( !isHasDirection ),
                "pop valid records failed" );
        Assert.assertEquals( popInvalidLogicalID(), -6,
                "pop invalid records success" );
        Assert.assertEquals( popHasNotLogicalID(), -6,
                "pop invalid records success" );
        Assert.assertEquals( popInvalidDirection(), -6,
                "pop invalid records success" );
    }

    public boolean popValidRecords( boolean isHasDirection ) {
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );

        int direction = 1;
        long logicalId = 0;

        try {
            logicalId = ( long ) cursor.getNext().get( "_id" );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }

        // pop the first record from CL
        BSONObject popObj = new BasicBSONObject();
        popObj.put( "LogicalID", logicalId );
        if ( isHasDirection ) {
            popObj.put( "Direction", direction );
        }
        cappedCL.pop( popObj );

        // remove the first elem from expectList
        for ( Iterator< BSONObject > it = insrtObjs.iterator(); it
                .hasNext(); ) {
            it.next();
            it.remove();
            break;
        }

        // check the records size
        long actBsonSize = cappedCL.getCount();
        long eptBsonSize = insrtObjs.size();
        Assert.assertEquals( actBsonSize, eptBsonSize, "actBsonSize: "
                + actBsonSize + " " + "eptBsonSize: " + eptBsonSize );

        // check every records
        DBCursor result = cappedCL.query( null, null, orderBy, null );
        int i = 0;
        try {
            while ( result.hasNext() && i < insrtObjs.size() ) {
                BSONObject act = result.getNext();
                BSONObject exp = insrtObjs.get( i );
                Assert.assertEquals( act.toString(), exp.toString(),
                        "act.toString(): " + act.toString() + " "
                                + "exp.toString(): " + exp.toString() );
                i++;
            }
        } finally {
            if ( result != null ) {
                result.close();
            }
        }
        return true;
    }

    public int popInvalidLogicalID() {
        long logicalId = 99999;
        int direction = 1;

        try {
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "LogicalID", logicalId );
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "logicalId 99999 error: " + e.getErrorCode()
                    + " " + e.getMessage() );
            return e.getErrorCode();
        }
    }

    public int popHasNotLogicalID() {
        int direction = 1;

        try {
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "logicalId none error: " + e.getErrorCode()
                    + " " + e.getMessage() );
            return e.getErrorCode();
        }
    }

    public int popInvalidDirection() {
        double direction = 1.1;
        long logicalId = 0;
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor cursor = cappedCL.query( null, null, orderBy, null );

        try {
            while ( cursor.hasNext() ) {
                logicalId = ( long ) cursor.getNext().get( "_id" );
                break;
            }
            BSONObject popObj = new BasicBSONObject();
            popObj.put( "LogicalID", logicalId );
            popObj.put( "Direction", direction );
            cappedCL.pop( popObj );
            return 0;
        } catch ( BaseException e ) {
            System.out.println( "direction 1.1 error: " + e.getErrorCode() + " "
                    + e.getMessage() );
            return e.getErrorCode();
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }
}
