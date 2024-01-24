package com.sequoiadb.lob.utils;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestException;

/**
 * FileName: Commlib.java public call function for test basicOperation
 * 
 * @author linsuqiang
 * @Date 2017.1.3
 * @version 1.00
 */
public class BigLobUtils {

    public static ArrayList< String > groupList;

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

    /**
     * get the MD5 of buff
     * 
     * @param inbuff
     *            the object of need to get the MD5
     * @return the MD5 value
     * @throws BaseException
     *             when failed
     */
    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";
        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else {
                throw new RuntimeException( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            throw new SdbTestException( "fail to get md5!" );
        }
        return value;
    }

    /**
     * build a byte array of specific length, which's content is random
     * 
     * @param length
     * @return byte[]
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] randomBytes = new byte[ length ];
        new Random().nextBytes( randomBytes );
        return randomBytes;
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

    public static ArrayList< String > getDataGroups( Sequoiadb sdb )
            throws BaseException {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            e.getStackTrace();
            throw e;
        }
        return groupList;
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
        cr.close();
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
