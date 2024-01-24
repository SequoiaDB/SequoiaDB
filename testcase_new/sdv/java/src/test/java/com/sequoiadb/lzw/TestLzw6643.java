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
 * @FileName:seqDB-6643:指定Compressed:false,CompressionType:"lzw"创建CL 创建CL，指定Compressed:false,CompressionType:"lzw"，覆盖如下场景（掉换参数顺序）：
 *                                                                   {Compressed:false,CompressionType:"lzw"}
 *                                                                   {CompressionType:"lzw",Compressed:false}
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6643 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6643";

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
                cs.createCollection( clName, ( BSONObject ) JSON.parse(
                        "{Compressed: false, CompressionType: 'lzw'}" ) );
                throw new BaseException( -10000,
                        "cl shouldn't been created successfully" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
            }
            try {
                cs.createCollection( clName, ( BSONObject ) JSON
                        .parse( "{Compression: 'lzw', Compressed: false}" ) );
                throw new BaseException( -10000,
                        "cl shouldn't been created successfully" );
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