package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-17264:固定集合中增删改记录，与读并发
 * @date 2019-1-17
 * @author yinzhen
 *
 */
@Test(groups = "ru")
public class Transaction17264 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cappedCS17264";
    private String clName = "cappedCL17264";
    private DBCollection cappedCL = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private BSONObject object = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cappedCL = sdb
                .createCollectionSpace( csName,
                        ( BSONObject ) JSON.parse( "{Capped:true}" ) )
                .createCollection( clName, ( BSONObject ) JSON
                        .parse( "{Capped:true, Size:1024}" ) );
        object = new BasicBSONObject();
        long oid = 0L;
        object.put( "_id", oid );
        object.put( "a", 1 );
        object.put( "b", 1 );
        cappedCL.insert( object );
        expList.add( object );
        object = new BasicBSONObject();
        oid = 64L;
        object.put( "_id", oid );
        object.put( "a", 2 );
        object.put( "b", 2 );
        cappedCL.insert( object );
        expList.add( object );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();

        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !sdb.isClosed() ) {
            sdb.dropCollectionSpace( csName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启并发事务
        cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
        cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1插入记录，并读记录走表扫描
        long oid = 128L;
        BSONObject object = new BasicBSONObject();
        object.put( "_id", oid );
        object.put( "a", 3 );
        object.put( "b", 3 );
        cl1.insert( object );
        expList.add( object );
        DBCursor recordsCursor = cl1.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2读记录走表扫描
        recordsCursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务1执行pop操作
        recordsCursor = cl1.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        cl1.pop( ( BSONObject ) JSON.parse( "{LogicalID:0, Direction:1}" ) );
        cl1.pop( ( BSONObject ) JSON
                .parse( "{LogicalID:" + oid + ", Direction:-1}" ) );
        expList.clear();
        expList.add( this.object );

        // 事务1读记录走表扫描
        recordsCursor = cl1.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2读记录走表扫描
        recordsCursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 回滚事务1，并读记录走表扫描
        db1.rollback();
        recordsCursor = cl1.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2读记录走表扫描
        recordsCursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtils.getReadActList( recordsCursor );
        Assert.assertEquals( actList, expList );

        // 事务2回滚
        db2.rollback();
        recordsCursor.close();
    }
}
