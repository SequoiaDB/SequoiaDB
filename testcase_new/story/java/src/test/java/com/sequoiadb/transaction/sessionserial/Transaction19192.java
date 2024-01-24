package com.sequoiadb.transaction.sessionserial;

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

/**
 * 
 * @description seqDB-19192:coord及数据节点均开启事务，TransAutoCommit属性不一致
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "ru")
public class Transaction19192 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19192";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName,
                ( BSONObject ) JSON
                        .parse( "{ShardingKey:{_id:1}, AutoSplit:true}" ) );
        sdb.updateConfig( ( BSONObject ) JSON.parse( "{transautocommit:true}" ),
                ( BSONObject ) JSON.parse( "{Global:false}" ) );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;

        try {

            // 执行插入，检查coord节点及数据节点debug日志
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            BSONObject obj = ( BSONObject ) JSON.parse( "{_id:1, a:1, b:1}" );
            cl1.insert( obj );

        } finally {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
            sdb.deleteConfig(
                    ( BSONObject ) JSON.parse( "{transautocommit:''}" ),
                    ( BSONObject ) JSON.parse( "{Global:false}" ) );
            sdb.close();
        }
    }
}
