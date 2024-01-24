package com.sequoiadb.lzw;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * @FileName:seqDB-6645:修改CL为关闭压缩 1、CL压缩类型为lzw，修改CL为关闭压缩，即指定Compressed:false
 *                                2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6645 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6645";
    private String dataGroupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dataGroupName = LzwUtils2.getDataGroups( sdb ).get( 0 );
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
    @Test(enabled = false)
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( csName );
            DBCollection cl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( "{Compressed: true, "
                            + "CompressionType: 'lzw', Group: '" + dataGroupName
                            + "'}" ) );
            cl.alterCollection(
                    ( BSONObject ) JSON.parse( "{Compressed:false}" ) );
            Assert.assertFalse( isCompressed( cl, dataGroupName ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private boolean isCompressed( DBCollection cl, String dataGroupName ) {
        Sequoiadb db = cl.getSequoiadb();
        Sequoiadb dataDB = LzwUtils3.getDataDB( db, dataGroupName );

        BSONObject nameBSON = new BasicBSONObject();
        nameBSON.put( "Name", csName + "." + clName );
        DBCursor cursor = dataDB.getSnapshot( 4, nameBSON, null, null );
        BasicBSONList details = ( BasicBSONList ) cursor.getNext()
                .get( "Details" );
        cursor.close();
        BSONObject detail = ( BSONObject ) details.get( 0 );

        String attr = ( String ) detail.get( "Attribute" );
        boolean isCompressed = attr.equals( "Compressed" );
        return isCompressed;
    }
}