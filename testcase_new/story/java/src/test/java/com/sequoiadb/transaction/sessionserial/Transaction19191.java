package com.sequoiadb.transaction.sessionserial;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * 
 * @description seqDB-19191:coord及数据节点均开启事务，TransLockWait属性不一致
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "ru")
public class Transaction19191 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19191";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName,
                ( BSONObject ) JSON
                        .parse( "{ShardingKey:{_id:1}, AutoSplit:true}" ) );
        sdb.updateConfig( ( BSONObject ) JSON.parse( "{transisolation:1}" ),
                ( BSONObject ) JSON.parse( "{Global:true}" ) );
        sdb.updateConfig(
                ( BSONObject ) JSON
                        .parse( "{translockwait:true,transactiontimeout:"
                                + TransUtils.transTimeoutSession + "}" ),
                ( BSONObject ) JSON.parse( "{Global:false}" ) );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;
        Sequoiadb db2 = null;

        try {

            // 开启事务1，插入记录R1
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db1 );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl1.insert( obj );
            List< BSONObject > expList = new ArrayList<>();
            expList.add( obj );

            // 开启事务2，查询记录R1
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            TransUtils.beginTransaction( db2 );
            DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            DBCursor cursor;
            try {
                cursor = cl2.query();
                TransUtils.getReadActList( cursor );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }

            // 提交所有事务
            db1.commit();
            db2.commit();
            cursor = cl2.query();
            List< BSONObject > actList = TransUtils.getReadActList( cursor );
            Assert.assertEquals( actList, expList );
        } finally {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
            if ( null != db2 ) {
                db2.commit();
                db2.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
            sdb.deleteConfig(
                    ( BSONObject ) JSON.parse( "{transisolation:''}" ),
                    ( BSONObject ) JSON.parse( "{Global:true}" ) );
            sdb.deleteConfig( ( BSONObject ) JSON.parse( "{translockwait:''}" ),
                    ( BSONObject ) JSON.parse( "{Global:false}" ) );
            sdb.close();
        }
    }
}
