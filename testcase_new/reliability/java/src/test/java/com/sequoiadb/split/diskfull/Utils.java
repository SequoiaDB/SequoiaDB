package com.sequoiadb.split.diskfull;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Utils {

    public static final String CATA_RG_NAME = "SYSCatalogGroup";

    public static int getBound( Sequoiadb commSdb, String clFullName,
            String srcGroupName, String destGroupName ) {
        DBCursor cursor = null;
        BSONObject lowBound = null;
        BSONObject upBound = null;
        try {
            cursor = commSdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + clFullName + "\"}", null, null );
            BasicBSONList list = null;
            if ( cursor.hasNext() ) {
                list = ( BasicBSONList ) cursor.getNext().get( "CataInfo" );
            } else {
                Assert.fail( clFullName + " collection catalog not found" );
            }
            for ( int i = 0; i < list.size(); i++ ) {
                String groupName = ( String ) ( ( BSONObject ) list.get( i ) )
                        .get( "GroupName" );
                if ( groupName.equals( destGroupName ) ) {
                    lowBound = ( BSONObject ) ( ( BSONObject ) list.get( i ) )
                            .get( "LowBound" );

                }
                if ( groupName.equals( srcGroupName ) ) {
                    upBound = ( BSONObject ) ( ( BSONObject ) list.get( i ) )
                            .get( "UpBound" );

                }
            }
            if ( !upBound.equals( lowBound ) ) {
                Assert.fail( "get lowbound upbound fail:" + list );
            }
            return ( int ) upBound.get( "sk" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        return ( int ) upBound.get( "UpBound" );

    }

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

    public static String getDiffHostWithSvc( String host,
            Set< String > allHost ) {
        for ( String entry : allHost ) {
            if ( !entry.equals( host ) ) {
                return entry + ":" + SdbTestBase.serviceName;
            }
        }
        return null;
    }

    public static String getString( int length ) {
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length / 16; i++ ) {
            sb.append( "SeedSeedSeedSeed" );
        }
        return sb.toString();
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
