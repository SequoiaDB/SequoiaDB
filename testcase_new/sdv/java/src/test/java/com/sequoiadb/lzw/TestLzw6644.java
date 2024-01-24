package com.sequoiadb.lzw;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6644:指定错误压缩类型创建CL 指定Compressed:true,CompressionType:"aaa"创建CL
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6644 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6644";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
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
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            try {
                cs.createCollection( clName, ( BSONObject ) JSON.parse(
                        "{Compressed: true, CompressionType: 'aaa'}" ) );
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