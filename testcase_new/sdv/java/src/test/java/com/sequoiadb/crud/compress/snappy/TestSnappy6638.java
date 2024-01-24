package com.sequoiadb.crud.compress.snappy;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @FileName:seqDB-6638: 存在索引，增删改查数据 1、CL压缩类型为snappy，且存在索引和已被压缩的记录，对CL做增删改查数据
 *                       2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-20
 * @Version 1.00
 */
public class TestSnappy6638 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6638";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        try {
            DBCollection cl = createCL();
            cl.createIndex( "aIndex", "{a:1}", false, false );
            SnappyUilts.insertData( cl, 100 );
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
        DBCollection cl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            // do insert
            cl.insert( "{a:-1, b:-1, c:-1}" );
            // check insert by query
            DBCursor cursor = cl.query( "{a:-1}", null, null, null );
            BSONObject result = cursor.getNext();
            result.removeField( "_id" );
            if ( !( result.equals( JSON.parse( "{a:-1, b:-1, c:-1}" ) ) ) ) {
                Assert.fail( "fail to insert" );
            }
            ;

            // do update
            cl.update( "{a:-1}", "{$set:{b:1}}", null );
            // check update by query
            cursor = cl.query( "{a:-1}", null, null, null );
            result = cursor.getNext();
            result.removeField( "_id" );
            if ( !( result.equals( JSON.parse( "{a:-1, b:1, c:-1}" ) ) ) ) {
                Assert.fail( "fail to update" );
            }
            ;

            // do delete
            cl.delete( "{a:-1}" );
            // check delete by query
            cursor = cl.query( "{a:-1}", null, null, null );
            if ( cursor.hasNext() ) {
                Assert.fail( "fail to delete" );
            }
            ;
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private DBCollection createCL() {
        DBCollection cl = null;
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        BSONObject option = new BasicBSONObject();
        try {
            option.put( "Compressed", true );
            option.put( "CompressionType", "snappy" );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }
}