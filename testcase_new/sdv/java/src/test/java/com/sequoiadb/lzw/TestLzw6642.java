package com.sequoiadb.lzw;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.compress.snappy.SnappyUilts;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6642:指定Compressed/CompressionType关键字错误创建CL 1、创建CL，指定的Compressed/CompressionType关键字错误，
 *                                                            如：指定Compress:true,CompressionType:"lzw"
 *                                                            ，压缩方式字段名错误
 *                                                            或Compressed:true,Compression:"lzw"
 *                                                            ，压缩算法字段名错误
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6642 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6642";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SnappyUilts.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @SuppressWarnings("deprecation")
    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( csName );
            try {
                cs.createCollection( clName, ( BSONObject ) JSON
                        .parse( "{Compress: true, CompressionType: 'lzw'}" ) );
                throw new BaseException( -10000,
                        "cl is created successfully with a wrong parameter 'Compress'" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
            }
            try {
                cs.createCollection( clName, ( BSONObject ) JSON
                        .parse( "{Compressed: true, Compression: 'lzw'}" ) );
                throw new BaseException( -10000,
                        "cl is created successfully with a wrong parameter 'Compression'" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }
}