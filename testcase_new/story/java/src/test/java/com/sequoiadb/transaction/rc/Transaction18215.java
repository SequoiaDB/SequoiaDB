package com.sequoiadb.transaction.rc;

/**
 * @testcase seqDB-18215 : 内置sql的事务提交基本功能 
 * @date 2019-4-10
 * @author zhao xiaoni
 **/
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction18215 extends SdbTestBase {
    private String clName = "cl_18215";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList< BSONObject >();
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private DBCursor cursor = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", false, false );
    }

    @Test
    public void Test() {
        List< BSONObject > insertR1s = TransUtils.insertRandomDatas( cl, 0,
                100 );

        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        for ( int i = 0; i < 100; i++ ) {
            db1.execUpdate( "insert into " + csName + "." + clName
                    + "(_id, a, b) values (" + ( 100 + i ) + "," + ( 100 + i )
                    + "," + ( 100 + i ) + ")" );
            db1.execUpdate( "update " + csName + "." + clName + " set _id = "
                    + ( 200 + i ) + ", a = " + ( 200 + i ) + ", b = "
                    + ( 200 + i ) + " where a = " + i );
            db1.execUpdate( "delete from " + csName + "." + clName
                    + " where a = " + ( 100 + i ) );
            expList.add( ( BSONObject ) JSON.parse( "{_id:" + ( 200 + i )
                    + ", a:" + ( 200 + i ) + ", b:" + ( 200 + i ) + "}" ) );
        }

        // 事务2表扫描记录
        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(NULL)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, insertR1s );

        // 事务2索引扫描记录
        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(a)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, insertR1s );

        db1.commit();

        // 事务2表扫描记录
        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(NULL)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 事务2索引扫描记录
        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(a)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        db2.commit();

        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(NULL)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        cursor = db2.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(a)*/" );
        Assert.assertEquals( actList, expList );
    }

    @AfterClass
    public void tearDown() {
        db1.commit();
        db2.commit();

        cursor.close();
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
