package com.sequoiadb.serial;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;

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

public class Utils {

    public static final int FLG_INSERT_CONTONDUP = 0x00000001;

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

}
