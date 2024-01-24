package com.sequoiadb.basicoperation;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
//import org.bson.types.Binary;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestInsertToBson7152.java test interface: insert (BSONObject
 * insertor), update (DBQuery query), delete (BSONObject matcher)
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestInsertToBson7152 extends SdbTestBase {
    private String clName = "cl_7152";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        createCL();
    }

    private void createCL() {
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
    }

    public void insertDatas() throws ParseException {
        try {
            Object oid = null;
            BasicBSONObject obj = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            BSONObject arr = new BasicBSONList();
            ObjectId id = new ObjectId( "53bb5667c5d061d6f579d0bb" );
            Pattern regex = Pattern.compile( "dh.*fj",
                    Pattern.CASE_INSENSITIVE );
            BSONObject regex1 = new BasicBSONObject();
            BSONObject numberlong = new BasicBSONObject();
            /*
             * String str = "hello world"; byte[] strArr = str.getBytes();
             * Binary bindata = new Binary(strArr);
             */
            obj.put( "_id", id );
            obj.put( "no", 1 );

            String str = "12345.067891234567890123456789";
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            BSONDecimal decimal2 = new BSONDecimal( str, 100, 30 );
            obj.put( "decimal2", decimal2 );

            numberlong.put( "$numberLong", "9223372036854775807" );
            obj.put( "numlong", numberlong );
            obj.put( "str", "str0" );
            obj.put( "null", "null" );
            subObj.put( "a", 1 );
            obj.put( "obj", subObj );
            arr.put( "0", 0 );
            arr.put( "1", "test" );
            arr.put( "2", 2.34 );
            obj.put( "arr", arr );
            obj.put( "tc", "企业级*分布" );
            obj.put( "boolt", true );
            obj.put( "boolf", false );
            obj.put( "binary", regex );
            regex1.put( "$regex", "^2001" );
            regex1.put( "$options", "i" );
            obj.put( "serial_num", regex1 );
            // TODO:add the binary type datas after increase equals()
            /*
             * obj.put("bindata",bindata); Binary tmpBin =
             * (Binary)obj.get("bindata"); byte[] orig = tmpBin.getData();
             * StringBuffer buff = new StringBuffer(); for (int i = 0; i <
             * orig.length; ++i){ buff.append(orig[i]);}
             * System.out.println("*** old val" + buff.toString() + "***");
             */
            Date now = new Date();
            obj.put( "date", now );
            // timestamp
            String mydate = "2014-07-01 12:30:30.124232";
            String dateStr = mydate.substring( 0, mydate.lastIndexOf( '.' ) );
            String incStr = mydate.substring( mydate.lastIndexOf( '.' ) + 1 );
            SimpleDateFormat format = new SimpleDateFormat(
                    "yyyy-MM-dd HH:mm:ss" );
            Date date = format.parse( dateStr );
            int seconds = ( int ) ( date.getTime() / 1000 );
            int inc = Integer.parseInt( incStr );
            BSONTimestamp ts = new BSONTimestamp( seconds, inc );
            obj.put( "timestamp", ts );
            oid = cl.insert( obj );

            // test the insert return value :oid
            BSONObject rObj = new BasicBSONObject();
            rObj.put( "_id", oid );
            Assert.assertEquals( 1, cl.getCount( rObj ),
                    "the insert return oid is wrong" );

            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
                /*
                 * Binary newval = (Binary)temp.get("bindata"); byte[] nval =
                 * newval.getData(); String out = new String(nval);
                 * System.out.println("***out is:***" + out); StringBuffer buff1
                 * = new StringBuffer(); for (int i = 0; i < nval.length; ++i){
                 * buff1.append(nval[i]);} System.out.println("*** new " +
                 * buff1.toString() + "***");
                 */
            }
            tmpCursor.close();
            Assert.assertEquals( actRecs.toString(), obj.toString(),
                    "check datas are unequal\n" + "actDatas: " + actRecs
                            + "\nexpected: " + obj + " \n"
                            + obj.get( "binary" ).getClass().getName() + " \n"
                            + actRecs.get( "binary" ).getClass().getName() );
            System.out.println( "---insert BsonTypeDatas is ok" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "insert BsonDatas fail " + e.getMessage() );
        }

    }

    // test update (DBQuery query)
    public void updateDatas() {
        try {
            DBQuery query = new DBQuery();
            BSONObject modifier = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            BSONObject contition = new BasicBSONObject();
            matcher.put( "str", "str0" );
            mValue.put( "no", 2147483646 );
            modifier.put( "$inc", mValue );
            query.setModifier( modifier );
            query.setMatcher( matcher );
            cl.update( query );
            contition.put( "no", 2147483647 );
            long count = cl.getCount( contition );
            Assert.assertEquals( 1, count );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update(DBQuery query) fail " + e.getMessage() );
        }
    }

    public void deleteDatas() {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            mValue.put( "a", 1 );
            matcher.put( "obj", mValue );
            cl.delete( matcher );

            long count = cl.getCount( matcher );
            Assert.assertEquals( 0, count, "the count is : " + count );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "delete (BSONObject matcher)  fail " + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    @Test
    public void testInsertToBson() throws ParseException {
        try {
            insertDatas();
            updateDatas();
            deleteDatas();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, e.getMessage() );
        }
    }
}
