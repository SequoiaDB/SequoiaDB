package com.sequoiadb.split.brokennetwork;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;

import java.util.ArrayList;
import java.util.List;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Utils {

    public static final String CATA_RG_NAME = "SYSCatalogGroup";

    // 检查某集合是否仅含一个dest记录
    public static boolean isCollectionContainThisJSON( DBCollection cl,
            String dest ) throws BaseException {
        BSONObject bobj = ( BSONObject ) JSON.parse( dest );
        ArrayList< Object > resaults = new ArrayList< Object >();
        DBCursor dc = null;
        try {
            dc = cl.query( bobj, null, null, null );
            while ( dc.hasNext() ) {
                resaults.add( dc.getNext() );
            }
            if ( resaults.size() != 1 ) {
                return false;
            }
            BSONObject actual = ( BSONObject ) resaults.get( 0 );
            actual.removeField( "_id" );
            bobj.removeField( "_id" );
            if ( bobj.equals( actual ) ) {
                return true;
            } else {
                return false;
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    // 获取异常的堆栈信息字串
    public static String getStackString( Exception e ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            stackBuffer.append( stackElements[ i ].toString() )
                    .append( "\r\n" );
        }
        String str = stackBuffer.toString();
        if ( str.length() >= 2 ) {
            return str.substring( 0, str.length() - 2 );
        } else {
            return str;
        }
    }

    public static void reelect( String destHost, String groupName1,
            String groupName2 ) throws ReliabilityException {
        List< GroupWrapper > groups = new ArrayList< GroupWrapper >();
        groups.add( GroupMgr.getInstance().getGroupByName( groupName1 ) );
        groups.add( GroupMgr.getInstance().getGroupByName( groupName2 ) );
        reelect( destHost, groups );
    }

    public static void reelect( String destHost, String groupName )
            throws ReliabilityException {
        List< GroupWrapper > groups = new ArrayList< GroupWrapper >();
        groups.add( GroupMgr.getInstance().getGroupByName( groupName ) );
        reelect( destHost, groups );
    }

    public static void reelect( String destHost, List< GroupWrapper > groups )
            throws ReliabilityException {
        for ( GroupWrapper group : groups ) {
            if ( destHost.equals( group.getMaster().hostName() )
                    && !group.changePrimary() ) {
                throw new SkipException(
                        group.getGroupName() + " failed to reelect" );
            }
        }
    }

    public static void waitSplit( Sequoiadb db, String clFullName )
            throws Exception {
        DBCursor cursor = null;
        try {
            int retryTimes = 300;
            int currTimes = 0;
            while ( currTimes < retryTimes ) {
                cursor = db.listTasks(
                        new BasicBSONObject( "Name", clFullName ).append(
                                "Status", new BasicBSONObject( "$et", 9 ) ),
                        null, null, null );
                if ( cursor.hasNext() ) {
                    break;
                } else {
                    Thread.sleep( 200 );
                    currTimes++;
                    if ( currTimes >= retryTimes ) {
                        Assert.fail( "Timeout get successful task." );
                    }
                }
            }
        } finally {
            cursor.close();
        }
    }
}
