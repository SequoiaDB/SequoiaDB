package com.sequoiadb.lzw;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.compress.snappy.SnappyUilts;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6646:修改CL的压缩类型为snappy 1、CL压缩类型为lzw，修改CL的压缩类型为snappy 2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6646 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6646";

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
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            DBCollection cl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( "{Compressed: true, "
                            + "CompressionType: 'lzw'}" ) );
            cl.alterCollection( ( BSONObject ) JSON
                    .parse( "{CompressionType: 'snappy'}" ) );
            Assert.assertEquals( "snappy", getCompressType( cl ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private String getCompressType( DBCollection cl ) {
        Sequoiadb db = cl.getSequoiadb();
        BSONObject cond = new BasicBSONObject( "Name", cl.getFullName() );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, cond,
                null, null );
        String compressType = ( String ) cursor.getNext()
                .get( "CompressionTypeDesc" );
        cursor.close();
        return compressType;
    }
}