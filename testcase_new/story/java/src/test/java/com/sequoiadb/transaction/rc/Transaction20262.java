package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20262:指定_id字段更新并发，删除记录后回滚
 * @date 2019-11-18
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction20262 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String clName = "cl20262";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = new Sequoiadb( coordUrl, "", "" );
        db2 = new Sequoiadb( coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        DBCollection cl = sdb.getCollectionSpace( csName )
                .createCollection( clName );
        cl.createIndex( "idx20262", "{a:1}", false, false );
        cl.insert( "{_id:1, a:1, b:1}" );
    }

    @AfterClass
    public void tearDown() {
        if ( db1 != null ) {
            db1.commit();
            db1.close();
        }
        if ( db2 != null ) {
            db2.commit();
            db2.close();
        }
        if ( sdb != null ) {
            sdb.commit();
            sdb.getCollectionSpace( csName ).dropCollection( clName );
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );

        // 事务1指定_id字段更新记录R1为R2；事务2指定_id字段更新记录R1为R3。
        cl1.update( "{_id:1}", "{$set:{a:2}}", "" );
        List< BSONObject > expList = new ArrayList<>();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" ) );
        TransUtils.queryAndCheck( cl1, "{_id:1}", expList );
        Trans2Update th2 = new Trans2Update();
        th2.start();
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, th2.getTransactionID() ) );

        // 事务1指定_id删除记录R2，然后回滚；事务2提交。
        cl1.delete( "{_id:1}" );
        db1.rollback();
        Assert.assertTrue( th2.isSuccess(), th2.getErrorMsg() );
        db2.commit();
        expList.clear();
        expList.add( ( BSONObject ) JSON.parse( "{_id:1, a:2, b:1}" ) );
        TransUtils.queryAndCheck( cl1, "{_id:1}", expList );
    }

    class Trans2Update extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( db2 );

            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            cl2.update( "{_id:1}", "{$set:{a:2}}", "" );
        }
    }
}