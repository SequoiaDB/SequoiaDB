package com.sequoiadb.subcl.diskfull;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.util.JSON;

import java.util.ArrayList;
import java.util.Set;

/**
 * 
 * @description Utils for this package class
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Utils {

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

}
