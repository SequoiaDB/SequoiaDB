package com.sequoiadb.decimal;

import java.math.BigInteger;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
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
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9587.java
 * TestLink: seqDB-9587
 * 
 * @author zhaoyu
 * @Date 2016.9.27
 * @version 1.00
 */
public class Decimal9587 extends SdbTestBase {
    private Sequoiadb sdb;

    private CollectionSpace cs = null;
    private String clName = "cl9587";
    private DBCollection cl = null;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
            cs = sdb.getCollectionSpace( commCSName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
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
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // insert int
        String[] valueArr = { "-2147483648", "2147483647", "-12.1122334455667",
                "1122334455667.12", "-9223372036854775808",
                "9223372036854775807" };
        int[] precisionArr = { 10, 15, 20, 40, 19, 30 };
        int[] scaleArr = { 0, 2, 18, 20, 0, 10 };
        int i = 0;
        BSONObject obj = null;
        BigInteger numberLong;
        try {
            for ( i = 0; i < valueArr.length; i++ ) {
                if ( i < 2 ) {
                    obj = new BasicBSONObject();
                    obj.put( "a", Integer.parseInt( valueArr[ i ] ) );
                } else if ( i < 4 ) {
                    obj = new BasicBSONObject();
                    obj.put( "a", Double.parseDouble( ( valueArr[ i ] ) ) );
                } else {
                    obj = new BasicBSONObject();
                    numberLong = new BigInteger( valueArr[ i ] );
                    obj.put( "a", numberLong.longValue() );

                }
                cl.insert( obj );
            }

            // insert decimal has precision and scale
            for ( i = 0; i < valueArr.length; i++ ) {
                obj = new BasicBSONObject();
                BSONDecimal decimal = new BSONDecimal( valueArr[ i ],
                        precisionArr[ i ], scaleArr[ i ] );
                obj.put( "a", decimal );
                cl.insert( obj );
            }
        } catch ( IllegalArgumentException e ) {
            Assert.fail( "generate decimal data:" + obj + " failed, errMsg:"
                    + e.getMessage() );
        } catch ( BaseException e ) {
            Assert.fail( "insert common data:" + obj + " failed, errMsg:"
                    + e.getMessage() );
        }

        // check
        for ( i = 0; i < valueArr.length; i++ ) {
            obj = new BasicBSONObject();
            BSONDecimal matcher = new BSONDecimal( valueArr[ i ] );
            obj.put( "a", matcher );
            DBCursor dbCursor = cl.query( obj, null, null, null );
            int count = 0;
            while ( dbCursor.hasNext() ) {
                count++;
                dbCursor.getNext();
            }
            dbCursor.close();
            Assert.assertEquals( count, 2 );
        }
    }
}
