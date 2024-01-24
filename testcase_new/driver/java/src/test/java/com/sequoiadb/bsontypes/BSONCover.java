package com.sequoiadb.bsontypes;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.types.Binary;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import static org.testng.Assert.assertEquals;

/**
 * seqDB-11898:data类型实现BSON和JSON类型转换 seqDB-11899:binary类型实现BSON和JSON类型转换
 * seqDB-11900:timestamp类型实现BSON和JSON类型转换
 */
public class BSONCover {

    /**
     * 验证: 1.使用Date构造BSONTimestamp,以及toDate接口
     * 2.使用Timestamp构造BSONTimestamp,以及toTimestamp接口
     */
    @Test
    public void testBSONTimestamp() {
        Date srcDate = new Date();
        BSONTimestamp ts = new BSONTimestamp( srcDate );
        Date toDate = ts.toDate();
        assertEquals( srcDate, toDate );

        Timestamp srcTS = new Timestamp( srcDate.getTime() );
        srcTS.setNanos( 123456000 );
        // nanoseconds loss
        BSONTimestamp ts2 = new BSONTimestamp( srcTS );
        Timestamp toTS = ts2.toTimestamp();
        assertEquals( srcTS, toTS );
    }

    /**
     * 验证：当BSONObject中存在Timestamp时，序列化前后的Timestamp是否相同
     */
    @Test
    public void testBSONCodecTimestamp() {
        Timestamp ts = new Timestamp( new Date().getTime() );
        ts.setNanos( 123456000 );

        BSONObject obj = new BasicBSONObject();
        obj.put( "ts", ts );

        byte[] bytes = BSON.encode( obj );
        BSONObject obj2 = BSON.decode( bytes );

        BSONTimestamp bts = ( BSONTimestamp ) obj2.get( "ts" );
        Timestamp ts2 = bts.toTimestamp();

        assertEquals( ts, ts2 );
    }

    /**
     * 测试用例 seqDB-11898 验证：JSON.parse()解析date时，是否使用当前时区.
     */
    @Test
    public void testJSONParseDate() {
        Date date = null;
        try {
            date = new SimpleDateFormat( "yyyy-MM-dd" ).parse( "2017-06-14" );
        } catch ( ParseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false );
        }
        java.sql.Date date2 = new java.sql.Date( date.getTime() );

        BSONObject obj = new BasicBSONObject();
        obj.put( "date", date );
        obj.put( "date2", date2 );

        String json = obj.toString();

        BSONObject obj2 = ( BSONObject ) JSON.parse( json );

        assertEquals( obj, obj2 );
    }

    /**
     * seqDB-11900 验证：Timestimp类型在BSONObject和JSON之间转换
     */
    @Test
    public void testJSONParseTimestamp() {
        Timestamp ts = new Timestamp( new Date().getTime() );
        ts.setNanos( 123456000 );

        BSONObject obj = new BasicBSONObject();
        obj.put( "ts", ts );

        BSONObject obj2 = new BasicBSONObject();
        obj2.put( "ts", new BSONTimestamp( ts ) );

        String json = JSON.serialize( obj );
        BSONObject obj3 = ( BSONObject ) JSON.parse( json );

        assertEquals( obj2, obj3 );
    }

    /**
     * seqDB-11899 验证：binary在序列化为JSON字符串时，是否转换为正确的二进制字符串，JSON.parse是否能正常解析。
     */
    @Test
    public void testJSONParseBinary() {
        String str = "hello world";
        Binary binary = new Binary( str.getBytes() );

        BSONObject obj = new BasicBSONObject();
        obj.put( "bin", binary );

        String json = obj.toString();
        BSONObject obj2 = ( BSONObject ) JSON.parse( json );

        assertEquals( obj, obj2 );
    }
}
