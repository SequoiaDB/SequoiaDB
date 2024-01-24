package com.sequoiadb.transaction.rc;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20215:事务中调用interrupt()
 * @date 2019-11-6
 * @author yinzhen
 *
 */
@Test(groups = "rc")
public class Transaction20215 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl20215";

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.commit();
            sdb.getCollectionSpace( csName ).dropCollection( clName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        TransUtils.beginTransaction( sdb );
        DBCollection cl1 = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 连接1开启事务，插入记录R1，然后调用interruptOperation接口，使用该连接继续插入记录R2。
        sdb.interruptOperation();
        cl1.insert( "{_id:1, a:1, b:1}" );
        BSONObject obj = ( BSONObject ) JSON.parse( "{_id:2, a:2, b:2}" );
        cl1.insert( obj );

        // 回滚该事务，查询所有记录。
        sdb.rollback();
        TransUtils.queryAndCheck( cl1, "{_id:1}",
                new ArrayList< BSONObject >() );
    }
}