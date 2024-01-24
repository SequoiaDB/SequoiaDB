package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.DateInterceptUtil;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestInsertRecords16658 insertRecords批量插入记录
 * @author wangkexin
 * @Date 2018-12-03
 * @version 1.00
 */

public class TestInsertRecords16658 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commcs = null;
    private DBCollection cl = null;
    private String clName = "cl_16658";
    private String coordAddr;

    @BeforeClass
    private void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        commcs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = commcs.createCollection( clName );
    }

    @Test
    public void testInsertRecords() {
        // 批量插入数据，覆盖所有数据类型，flag取值为0和FLG_INSERT_RETURN_OID
        InsertRecords();

        // 存在重复数据，flag取值为0和FLG_INSERT_CONTONDUP
        InsertRecordsDuplicateKey();

        // flag取值不正确
        InsertRecordsFlagError( -1 );

        // 设置ensureOID()为false，flag取值为FLG_INSERT_RETURN_OID
        InsertRecordsSetEnsureOid();
    }

    @AfterClass
    private void tearDown() {
        commcs.dropCollection( clName );
        sdb.close();
    }

    private void InsertRecords() {
        int num = 2;
        List< BSONObject > list = new ArrayList< BSONObject >();
        String[] oidArray = new String[ num ];
        for ( int i = 0; i < num; i++ ) {
            BSONObject obj = new BasicBSONObject();
            ObjectId id = new ObjectId();
            obj.put( "_id", id );
            oidArray[ i ] = id.toString();
            // insert the decimal type data
            String str = "32345.067891234567890123456789" + i;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            obj.put( "no", i );
            obj.put( "str", "test_" + String.valueOf( i ) );
            // the numberlong type data
            BSONObject numberlong = new BasicBSONObject();
            numberlong.put( "$numberLong", "-9223372036854775808" );
            obj.put( "numlong", numberlong );
            // the obj type
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 + i );
            obj.put( "obj", subObj );
            // the array type
            BSONObject arr = new BasicBSONList();
            arr.put( "0", ( int ) ( Math.random() * 100 ) );
            arr.put( "1", "test" );
            arr.put( "2", 2.34 );
            obj.put( "arr", arr );
            obj.put( "boolf", false );
            // the data type
            Date now = new Date();
            Date expdate = DateInterceptUtil.interceptDate( now, "yyyy-MM-dd" );
            obj.put( "date", expdate );
            // the regex type
            Pattern regex = Pattern.compile( "^2001",
                    Pattern.CASE_INSENSITIVE );
            obj.put( "binary", regex );
            list.add( obj );
        }

        // test a : flag is 0
        cl.insertRecords( list, 0 );

        // check the InsertRecords result
        BSONObject tmp = new BasicBSONObject();
        DBCursor tmpCursor = cl.query( tmp, null, null, null );
        BasicBSONObject temp = null;
        List< BSONObject > listActDatas = new ArrayList< BSONObject >();
        while ( tmpCursor.hasNext() ) {
            temp = ( BasicBSONObject ) tmpCursor.getNext();
            listActDatas.add( temp );
        }
        Assert.assertEquals( listActDatas.equals( list ), true,
                "check datas are unequal\n" + "actDatas: "
                        + listActDatas.toString() );

        cl.truncate();
        // test b : flag is FLG_INSERT_RETURN_OID
        BSONObject returnedOid = cl.insertRecords( list,
                DBCollection.FLG_INSERT_RETURN_OID );
        Assert.assertEquals( returnedOid.get( "_id" ).toString(),
                "[" + oidArray[ 0 ] + ", " + oidArray[ 1 ] + "]" );
    }

    private void InsertRecordsDuplicateKey() {
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( long i = 0; i < 2; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "_id", 1 );
            String str = "32345.067891234567890123456789";
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            obj.put( "no", 5 );
            list.add( obj );
        }
        // test a : flag is 0
        try {
            cl.insertRecords( list, 0 );
            Assert.fail(
                    "bulk insert will interrupt when Duplicate key exist" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }
        Assert.assertEquals( cl.getCount( list.get( 0 ) ), 1,
                "the actDatas is :" + cl.getCount( list.get( 0 ) ) );

        cl.truncate();
        BSONObject obj = new BasicBSONObject();
        obj.put( "_id", 2 );
        list.add( obj );
        // test b : flag is FLG_INSERT_CONTONDUP
        cl.insertRecords( list, DBCollection.FLG_INSERT_CONTONDUP );
        Assert.assertEquals( cl.getCount( list.get( 0 ) ), 1,
                "the actDatas is :" + cl.getCount( list.get( 0 ) ) );

        // 重复记录跳过不插入，继续插入后面的记录
        Assert.assertEquals( cl.getCount( list.get( 2 ) ), 1,
                "the actDatas is :" + cl.getCount( list.get( 2 ) ) );
    }

    private void InsertRecordsFlagError( int flag ) {
        List< BSONObject > list = new ArrayList< BSONObject >();
        BSONObject obj = new BasicBSONObject();
        obj.put( "no", flag );
        list.add( obj );
        try {
            cl.insertRecords( list, flag );
            Assert.fail( "Illegal flag insert failed!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
        }
        Assert.assertEquals( cl.getCount( obj ), 0, "when flag is " + flag
                + " , the actDatas is :" + cl.getCount( obj ) );
    }

    private void InsertRecordsSetEnsureOid() {
        List< BSONObject > list = new ArrayList< BSONObject >();
        BSONObject obj = new BasicBSONObject();
        ObjectId oid = new ObjectId();
        obj.put( "_id", oid );
        obj.put( "testEnsureOid", 123 );
        list.add( obj );

        cl.ensureOID( false );
        BSONObject returnedOid = cl.insertRecords( list,
                DBCollection.FLG_INSERT_RETURN_OID );
        Assert.assertEquals( returnedOid.get( "_id" ).toString(),
                "[" + oid + "]" );
        Assert.assertEquals( cl.getCount( obj ), 1,
                "the actDatas is :" + cl.getCount( obj ) );
    }
}
