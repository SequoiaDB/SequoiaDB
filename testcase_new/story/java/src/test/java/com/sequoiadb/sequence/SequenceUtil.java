package com.sequoiadb.sequence;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

public class SequenceUtil {

    private static List< BSONObject > getSnapshotResult( Sequoiadb db,
            BSONObject matcher, BSONObject selector ) {
        DBCursor cursor = null;
        List< BSONObject > actualResult = new ArrayList<>();
        cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES, matcher,
                selector, null );
        while ( cursor.hasNext() ) {
            actualResult.add( cursor.getNext() );
        }
        cursor.close();
        return actualResult;
    }

    public static void checkSequence( Sequoiadb db, String seqName,
            List< BSONObject > exprList ) throws Exception {
        List< BSONObject > actualList = getSnapshotResult( db,
                ( BSONObject ) JSON.parse( "{'Name':'" + seqName + "'}" ),
                null );

        if ( actualList.size() != exprList.size() ) {
            throw new Exception( "lists don't have the same size expected ["
                    + exprList.size() + "]but found [" + actualList.size()
                    + "],actualList" + actualList.toString() + ";exprList:"
                    + exprList.toString() );
        }
        for ( int i = 0; i < actualList.size(); i++ ) {
            for ( String fieldName : exprList.get( i ).keySet() ) {
                Assert.assertEquals( actualList.get( i ).get( fieldName ),
                        exprList.get( i ).get( fieldName ),
                        "actual:["
                                + actualList.get( i ).get( fieldName )
                                        .toString()
                                + "];expect:["
                                + exprList.get( i ).get( fieldName ) + "]" );
            }
        }
    }
}
