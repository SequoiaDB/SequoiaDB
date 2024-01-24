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
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-22099:设置优先走备节点，执行count/find查询
 * @date 2020.4.22
 * @author zhaoyu
 *
 */
@Test(groups = { "rc", "rr" })
public class Transaction22099 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl22099";
    private List< BSONObject > expList = new ArrayList<>();
    private DBCollection cl;

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        cl.createIndex( "a22099", "{a:1}", false, false );

        BSONObject record = ( BSONObject ) JSON.parse( "{_id:1,a:1,b:1}" );
        expList.add( record );
        cl.insert( record );
    }

    @AfterClass
    public void tearDown() {
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        sdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance:'S'}" ) );
        try {
            TransUtils.beginTransaction( sdb );
            int recordNum = ( int ) cl.getCount();
            Assert.assertEquals( recordNum, 1 );

            TransUtils.queryAndCheck( cl, "{'':'a22099'}", expList );
            TransUtils.queryAndCheck( cl, "{'':null}", expList );
        } finally {
            sdb.commit();
        }

    }

}
