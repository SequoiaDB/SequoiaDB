package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-17048
 * @describe: exec/msg接口参数支持中文校验
 * @author wangkexin
 * @Date 2019.01.03
 * @version 1.00
 */
public class TestExecAndMsg17048 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl17048";
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        this.cl = this.cs.createCollection( clName );
    }

    @Test
    public void test() {
        testExec();
        testMsg();
    }

    @AfterClass
    public void tearDown() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.sdb.close();
    }

    private void testExec() {
        List< BSONObject > objList = new ArrayList<>();
        BSONObject obj = new BasicBSONObject();
        obj.put( "test", ObjectId.get() );
        objList.add( obj );

        obj = new BasicBSONObject();
        obj.put( "test17048_1", "testtest" );
        obj.put( "test17048_2", "testtesttest" );
        objList.add( obj );

        BSONObject expObj = new BasicBSONObject();
        expObj.put( "testChinese", "测试中文" );
        objList.add( expObj );

        cl.insert( objList );

        String sql = String.format(
                "select * from %s where testChinese <> '' and testChinese = '测试中文' limit 3",
                cl.getFullName() );
        DBCursor cursor = sdb.exec( sql );
        Assert.assertTrue( cursor.hasNext() );
        Assert.assertEquals( cursor.getNext(), expObj );
        Assert.assertFalse( cursor.hasNext() );
        cursor.close();
    }

    private void testMsg() {
        List< String > messages = new ArrayList<>();
        messages.add( "Sequoaidb" );
        messages.add( "巨杉数据库" );
        messages.add( "巨杉数据库Sequoaidb" );
        for ( String message : messages ) {
            sdb.msg( message );
        }
    }
}
