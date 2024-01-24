package com.sequoiadb.basicoperation;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-28328:encode接口验证
 * @author wuyan
 * @Date 2022.10.19
 * @version 1.00
 */
public class TestInsert28328 extends SdbTestBase {
    private String OID = "_id";

    @BeforeClass
    public void setUp() {
    }

    @Test(enabled = false)
    public void test() {
        // obj源始数据覆盖存在oid和不存在oid
        BSONObject obj = new BasicBSONObject();
        obj.put( "a", 1 );
        encodeAndCheck( obj, null );

        obj.put( OID, ObjectId.get() );
        encodeAndCheck( obj, null );

        BSONObject obj1 = new BasicBSONObject();
        obj1.put( "a", 1 );

        BSONObject extendObj = new BasicBSONObject();
        extendObj.put( OID, ObjectId.get() );
        encodeAndCheck( obj1, extendObj );

        // 测试obj为子类BasicBSONList类型
        BSONObject value = new BasicBSONObject();
        BSONObject listObj = new BasicBSONList();
        value.put( "0", "test" );
        value.put( "0", 1 );
        listObj.putAll( value );
        encodeAndCheck( listObj, null );

        // 测试encode(obj, extendObj)两个参数类型不一致时不兼容报错
        // extendObj为list类型
        try {
            encodeAndCheck( obj, listObj );
            Assert.fail( "extendObj should be BasicBSONObject!" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue(
                    e.getMessage()
                            .contains( "extendObj should be BasicBSONObject" ),
                    e.getMessage() );
        }

        // obj和extendObj都为list类型
        try {
            encodeAndCheck( listObj, listObj );
            Assert.fail( "extendObj should be BasicBSONObject!" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue(
                    e.getMessage().contains(
                            "BasicBSONList does not support extended data" ),
                    e.getMessage() );
        }

        // obj 和extendObj 类型不一致
        try {
            encodeAndCheck( listObj, obj );
            Assert.fail( "extendObj should be BasicBSONObject!" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue(
                    e.getMessage().contains(
                            "BasicBSONList does not support extended data" ),
                    e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {

    }

    private void encodeAndCheck( BSONObject obj, BSONObject extendObj ) {
        BSONObject expected = new BasicBSONObject();
        expected.putAll( obj );
        if ( extendObj != null ) {
            expected.putAll( extendObj );
        }
        if ( obj.containsField( OID ) ) {
            expected.put( OID, obj.get( OID ) );
        }

//        byte[] bytes = BSON.encode( obj, extendObj );
//        Assert.assertEquals( expected, BSON.decode( bytes ) );
    }
}