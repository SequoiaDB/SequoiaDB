package com.sequoiadb.transaction.rc;

/**
 * @testcase seqDB-18219:内置sql删除失败回滚功能验证 
 * @date 2019-4-11
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

@Test(groups = "rc")
public class Transaction18219 extends SdbTestBase {
    private String clName = "cl_18219";
    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private DBCollection cl = null;
    private DBCursor cursor = null;
    private List< BSONObject > actList = new ArrayList< BSONObject >();
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        db2.setSessionAttr( ( BSONObject ) JSON.parse(
                "{TransTimeout:" + TransUtils.transTimeoutSession + "}" ) );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a", "{a:1}", true, false );
    }

    @Test
    public void Test() {
        sdb.execUpdate( "insert into " + csName + "." + clName
                + "(_id, a, b) values (1, 1, 1)" );

        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        db1.execUpdate( "update " + csName + "." + clName
                + " set _id = 2, a = 2, b = 2 where a = 1" );
        expList.add( ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" ) );

        db2.execUpdate( "insert into " + csName + "." + clName
                + "(_id, a, b) values (3, 3, 3)" );
        try {
            db2.execUpdate(
                    "delete from " + csName + "." + clName + " where a = 1" );
            throw new BaseException( -999, "Delete Record ERR" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -13 ) {
                throw e;
            }
        }

        cursor = sdb.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(NULL)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        cursor = sdb.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(a)*/" );
        Assert.assertEquals( actList, expList );

        db1.commit();

        cursor = sdb.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(NULL)*/" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        cursor = sdb.exec( "select * from " + csName + "." + clName
                + " order by a /*+use_index(a)*/" );
        Assert.assertEquals( actList, expList );
    }

    @AfterClass
    public void tearDown() {
        cursor.close();

        db1.commit();
        db2.commit();
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
