package com.sequoiadb.metaopr;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.exception.ReliabilityException;
import org.testng.Assert;

public class Utils {
    public static void checkConsistency( GroupMgr groupMgr )
            throws ReliabilityException, InterruptedException {
        groupMgr.refresh();

        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        List< String > urls = cataGroup.getAllUrls();

        int retryTimes = 0;
        int sleepTimes = 300; // ms
        int maxRetryTimes = 50;
        List< List< BSONObject > > resList = null;
        while ( true ) {
            retryTimes++;
            Thread.sleep( sleepTimes );
            resList = new ArrayList< List< BSONObject > >();

            // get catalog info from all catalog nodes
            for ( String url : urls ) {
                Sequoiadb cataDB = new Sequoiadb( url, "", "" );
                DBCollection cl = cataDB.getCollectionSpace( "SYSCAT" )
                        .getCollection( "SYSCOLLECTIONS" );
                DBCursor cursor = cl.query( null, null,
                        ( BSONObject ) JSON.parse( "{ _id: 1 }" ), null );
                List< BSONObject > res = new ArrayList< BSONObject >();
                while ( cursor.hasNext() ) {
                    res.add( cursor.getNext() );
                }
                cursor.close();
                resList.add( res );
                cataDB.close();
            }

            // check catalog count
            if ( resList.get( 0 ).size() == resList.get( 1 ).size()
                    && resList.get( 1 ).size() == resList.get( 2 ).size() ) {
                break;
            } else if ( retryTimes >= maxRetryTimes ) {
                System.out.println(
                        resList.get( 0 ).size() + " " + resList.get( 1 ).size()
                                + " " + resList.get( 2 ).size() );
                throw new ReliabilityException(
                        "Failed to check count between catalog nodes!" );
            }
        }

        // check catalog content
        List< BSONObject > srcList = resList.get( 0 );
        for ( int i = 1; i < resList.size(); i++ ) {
            List< BSONObject > dstList = resList.get( i );
            for ( int j = 0; j < srcList.size(); j++ ) {
                if ( !srcList.get( j ).equals( dstList.get( j ) ) ) {
                    System.out.println(
                            urls.get( 0 ) + " : " + srcList.get( j ) );
                    System.out.println(
                            urls.get( i ) + " : " + dstList.get( j ) );
                    throw new ReliabilityException(
                            "Failed to check records between catalog nodes!" );
                }
            }
        }
    }

    public static String getKeyStack( Exception e, Object classObj ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            if ( stackElements[ i ].toString()
                    .contains( classObj.getClass().getName() ) ) {
                stackBuffer.append( stackElements[ i ].toString() )
                        .append( "\r\n" );
            }
        }
        String str = stackBuffer.toString();
        if ( str.length() >= 2 ) {
            return str.substring( 0, str.length() - 2 );
        } else {
            return str;
        }
    }

    public static void dropCollectionSpace( String csName ) {
        int timeout = 600;
        int doTimes = 0;
        // 删除集合空间，如果报错-147则重试 10min
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            while ( doTimes < timeout ) {
                try {
                    db.dropCollectionSpace( csName );
                    // 删除cs成功，则退出
                    break;
                } catch ( BaseException e ) {
                    // 错误码为-147则进行重试
                    if ( e.getErrorCode() == SDBError.SDB_LOCK_FAILED
                            .getErrorCode() ) {
                        try {
                            Thread.sleep( 1000 );
                        } catch ( InterruptedException e2 ) {
                            e2.printStackTrace();
                        }
                        doTimes++;
                        continue;
                        // 错误码为-34则跳出循环
                    } else if ( e.getErrorCode() == SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                        break;
                    } else {
                        throw e;
                    }

                }
            }

            if ( doTimes >= timeout ) {
                Assert.fail("drop collection space time out");
            }
        }
    }
}
