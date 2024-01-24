package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SplitUtils {

    public static final int FLG_INSERT_CONTONDUP = 0x00000001;
    private static ArrayList< String > groupList;

    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "run mode is standalone" );
                return true;
            }
        }
        return false;
    }

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    public static CollectionSpace createCS( String csName, Sequoiadb db )
            throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static CollectionSpace createCS( String csName, Sequoiadb db,
            String option ) throws BaseException {
        CollectionSpace tmp = null;
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            tmp = db.createCollectionSpace( csName,
                    ( BSONObject ) JSON.parse( option ) );

        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
    }

    public static Domain createDomain( Sequoiadb sdb, String name,
            ArrayList< String > groupArr, int size, boolean autoSplit )
            throws BaseException {
        Domain domain = null;
        try {
            if ( sdb.isDomainExist( name ) ) {
                domain = sdb.getDomain( name );
            } else {
                StringBuilder groups = new StringBuilder();
                String option = new String();
                for ( int i = 0; i < groupArr.size() && i < size; i++ ) {
                    groups.append( "\"" ).append( groupArr.get( i ) )
                            .append( "\"," );
                }
                groups.deleteCharAt( groups.length() - 1 );
                groups.insert( 0, "[" );
                groups.append( "]" );
                if ( autoSplit ) {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":true}";
                } else {
                    option = "{\"Groups\":" + groups + ",\"AutoSplit\":false}";
                }
                domain = sdb.createDomain( name,
                        ( BSONObject ) JSON.parse( option ) );
            }

        } catch ( BaseException e ) {
            throw e;
        }
        return domain;
    }

    public static DBCollection createCL( String clName, CollectionSpace cs,
            String option ) throws BaseException {
        DBCollection tmp = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            tmp = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            throw e;
        }
        return tmp;
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

    public static ArrayList< String > getGroupName( Sequoiadb sdb,
            String csName, String clName ) throws BaseException {
        DBCursor dbc = null;
        ArrayList< String > resault = new ArrayList< String >();
        try {
            CommLib commlib = new CommLib();
            ArrayList< String > groups = commlib.getDataGroupNames( sdb );
            dbc = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                    "{Name:\"" + csName + "." + clName + "\"}", null, null );
            BasicBSONList list = null;
            if ( dbc.hasNext() ) {
                list = ( BasicBSONList ) dbc.getNext().get( "CataInfo" );
            } else {
                return null;
            }
            String srcGroupName = ( String ) ( ( BSONObject ) list.get( 0 ) )
                    .get( "GroupName" );
            resault.add( srcGroupName );
            if ( groups.size() < 2 ) {
                return resault;
            }
            String destGroupName;
            if ( srcGroupName.equals( groups.get( 0 ) ) )
                destGroupName = groups.get( 1 );
            else
                destGroupName = groups.get( 0 );
            resault.add( destGroupName );
            return resault;
        } catch ( BaseException e ) {
            throw e;
        } finally {
            if ( dbc != null ) {
                dbc.close();
            }
        }
    }

    /**
     * generating byte to write lob
     * 
     * @param length
     *            generating byte stream size
     * @return String sb
     */
    public static String getRandomString( int length ) {
        String base = "abcdefahijklmnopqrstuvwxyz0123456789";
        Random random = new Random();

        int times = length / 1024;
        if ( times < 1 ) {
            times = 1;
        }

        int len = length >= 1024 ? 1024 : 0;
        int mulByteNum = length % 1024;

        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < times; ++i ) {
            if ( i == times - 1 ) {
                len += mulByteNum;
            }

            char ch = base.charAt( random.nextInt( base.length() ) );
            for ( int k = 0; k < len; ++k ) {
                sb.append( ch );
            }

        }
        return sb.toString();
    }

    public static String getSrcGroupName( Sequoiadb sdb, String csName,
            String clName ) {
        String groupName = "";
        String cond = String.format( "{Name:\"%s.%s\"}", csName, clName );
        DBCursor cr = sdb.getSnapshot( 8, cond, null, null );
        while ( cr.hasNext() ) {
            BSONObject obj = cr.getNext();

            BasicBSONObject doc = ( BasicBSONObject ) obj;
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( 0 );
            groupName = elem.getString( "GroupName" );
        }
        return groupName;
    }

    public static String getSplitGroupName( String groupName ) {
        String tarRgName = "";
        for ( int i = 0; i < groupList.size(); ++i ) {
            String name = groupList.get( i );
            if ( !name.equals( groupName ) ) {
                tarRgName = name;
                break;
            }
        }
        return tarRgName;

    }

}
