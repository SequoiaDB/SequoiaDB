package com.sequoiadb.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestInsert16659 insert指定flag值插入单条记录
 * @author wangkexin
 * @Date 2018-12-03
 * @version 1.00
 */

public class TestInsertFlags16659 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commcs = null;
    private DBCollection cl = null;
    private String clName = "cl_16659";
    private String coordAddr;
    private String OID = "_id";
    private String indexName = "index16659";

    @BeforeClass
    private void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        commcs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = commcs.createCollection( clName );
    }

    @Test
    private void testInsert() {
        BSONObject obj = new BasicBSONObject();
        ObjectId id = new ObjectId();
        obj.put( OID, id );
        obj.put( "str", "hello" );
        obj.put( "int", 123 );
        obj.put( "double", 1234.567 );

        // case 1: flag取值为0 (_id字段存在唯一索引)
        cl.insert( obj, 0 );
        Assert.assertEquals( cl.getCount( obj ), 1 );

        // case 2: 非_id字段创建唯一索引，存在重复记录，设置flag取值为0
        BSONObject indexObj = ( BSONObject ) JSON.parse( "{str:1}" );
        cl.createIndex( indexName, indexObj, true, true );
        try {
            cl.insert( obj, 0 );
            Assert.fail( "expect fail but found success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }
        Assert.assertEquals( cl.getCount( obj ), 1 );
        cl.dropIndex( indexName );

        // case 3: flag 取值为 FLG_INSERT_CONTONDUP
        cl.insert( obj, DBCollection.FLG_INSERT_CONTONDUP );
        Assert.assertEquals( cl.getCount( obj ), 1 );

        // case 4: flag 取值为 FLG_INSERT_RETURN_OID
        ObjectId id2 = new ObjectId();
        BSONObject obj2 = new BasicBSONObject();
        obj2.put( OID, id2 );
        obj2.put( "a", 1 );
        BSONObject result2 = cl.insert( obj2,
                DBCollection.FLG_INSERT_RETURN_OID );
        Assert.assertEquals( result2.get( OID ), id2 );
        Assert.assertEquals( cl.getCount( obj2 ), 1 );

        // case 5: flag 取值为
        // FLG_INSERT_REPLACEONDUP,先插入一条不与obj冲突的记录，再插入一条与obj的"_id"索引键冲突的记录,插入后原记录obj不存在
        BSONObject obj3 = new BasicBSONObject();
        obj3.put( OID, "123456" );
        obj3.put( "str", "string" );
        cl.insert( obj3 );

        BSONObject obj3_dup = new BasicBSONObject();
        obj3_dup.put( OID, id );
        obj3_dup.put( "str", "hello2" );
        obj3_dup.put( "int", 456 );
        obj3_dup.put( "double", 1234.999 );
        obj3_dup.put( "appendField", "test16559" );
        cl.insert( obj3_dup, DBCollection.FLG_INSERT_REPLACEONDUP );
        Assert.assertEquals( cl.getCount( obj3 ), 1 );
        Assert.assertEquals( cl.getCount( obj3_dup ), 1 );
        Assert.assertEquals( cl.getCount( obj ), 0 );

        // case 6: flag 值为非法值如：-1
        BSONObject obj6 = new BasicBSONObject().append( "test3", 123 );
        try {
            cl.insert( obj6, -1 );
            Assert.fail( "Illegal flag insert failed!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
        }
        Assert.assertEquals( cl.getCount( obj6 ), 0 );
    }

    @AfterClass
    private void tearDown() {
        try {
            commcs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }
}
