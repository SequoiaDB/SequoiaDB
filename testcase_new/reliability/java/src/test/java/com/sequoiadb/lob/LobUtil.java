package com.sequoiadb.lob;

import static com.sequoiadb.metaopr.commons.MyUtil.createCS;
import static com.sequoiadb.metaopr.commons.MyUtil.createCl;
import static com.sequoiadb.metaopr.commons.MyUtil.getSdb;
import static com.sequoiadb.metaopr.commons.MyUtil.isCsExisted;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public class LobUtil {

    public static void createLobCsAndCl( String csName, String clName ) {
        Sequoiadb db = getSdb();
        if ( isCsExisted( csName ) == true ) {
            if ( db.getCollectionSpace( csName )
                    .getCollection( clName ) != null )
                return;
        }
        createCS( csName );
        BSONObject option = ( BSONObject ) JSON.parse(
                "{ ShardingKey: { \"age\": 1 }," + " ShardingType: \"hash\", "
                        + "Partition: 1024, ReplSize: 1,"
                        + " Compressed: true ," + "Group:\"group1\"}" );
        createCl( csName, clName, option );
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );

        cl.split( "group1", "group2", 50 );
        db.close();
    }

    /**
     * get the buff MD5 value
     * 
     * @param inbuff
     *            the object of need to get the MD5
     * @return the MD5 value
     */
    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
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

    /**
     * generating byte to write lob
     *
     * @param length
     *            generating byte stream size
     * @return byte[] bytes
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    /**
     * 创建lob， 默认插入20个lob，每个lob间隔1天
     * 
     * @param cl
     * @param data
     * @return List<ObjectId> lobIds
     */
    public static List< ObjectId > createAndWriteLob( DBCollection cl,
            byte[] data ) {
        Calendar cal = Calendar.getInstance();
        cal.set( Calendar.DATE, 1 );
        Date date = cal.getTime();
        SimpleDateFormat sf = new SimpleDateFormat( "yyyyMMdd" );
        return createAndWriteLob( cl, data, "YYYYMMDD", 20, 1,
                sf.format( date ) );
    }

    /**
     * 创建lob
     * 
     * @param cl
     *            集合
     * @param data
     *            数据
     * @param format
     *            主表分区键日期格式
     * @param lobNum
     *            创建的lob数
     * @param timeInterval
     *            lob间隔时间
     * @param beginDate
     *            构造lob的时间起始值
     * @return List<ObjectId> lobIds
     */
    public static List< ObjectId > createAndWriteLob( DBCollection cl,
            byte[] data, String format, int lobNum, int timeInterval,
            String beginDate ) {
        List< ObjectId > idList = new ArrayList< ObjectId >();
        int year = Integer.valueOf( beginDate.substring( 0, 4 ) );
        int month = Integer.valueOf( beginDate.substring( 4, 6 ) );
        int day = Integer.valueOf( beginDate.substring( 6, 8 ) );
        for ( int i = 0; i < lobNum; i++ ) {
            Calendar cal = Calendar.getInstance();
            switch ( format ) {
            case "YYYY":
                cal.set( Calendar.YEAR, year + i );
                cal.set( Calendar.MONTH, month - 1 );
                cal.set( Calendar.DATE, day );
                break;
            case "YYYYMM":
                cal.set( Calendar.YEAR, year );
                cal.set( Calendar.MONTH, month + i - 1 );
                cal.set( Calendar.DATE, day );
                break;
            default:
                cal.set( Calendar.YEAR, year );
                cal.set( Calendar.MONTH, month - 1 );
                cal.set( Calendar.DATE, day + i );
                break;
            }
            Sequoiadb sdb = cl.getSequoiadb();
            try {
                Date date = cal.getTime();
                ObjectId lodID = cl.createLobID( date );
                DBLob lob = cl.createLob( lodID );
                lob.write( data );
                lob.close();
                idList.add( lodID );
            } catch ( BaseException e ) {
                DBCursor cur = sdb.getSnapshot( 8,
                        new BasicBSONObject( "Name", cl.getFullName() ), null,
                        null );
                if ( cur.hasNext() ) {
                    System.out.println( cur.getNext().toString() );
                } else {
                    System.out.println(
                            cl.getFullName() + " not snapshot message." );
                }
                throw e;
            }
        }
        return idList;
    }

    /**
     * 检查lob md5是否正确
     * 
     * @param cl
     * @param lobIds
     * @param expData
     * @return booelan
     */
    public static boolean checkLobMD5( DBCollection cl, List< ObjectId > lobIds,
            byte[] expData ) {
        String expMD5 = LobUtil.getMd5( expData );
        for ( ObjectId lobId : lobIds ) {
            byte[] data = new byte[ expData.length ];
            DBLob lob = cl.openLob( lobId );
            lob.read( data );
            lob.close();
            String actMD5 = LobUtil.getMd5( data );
            if ( !actMD5.equals( expMD5 ) ) {
                throw new BaseException( 0, "check lob: " + lobId
                        + " md5 error, exp: " + expMD5 + ", act: " + actMD5 );
            }
        }
        return true;
    }
}
