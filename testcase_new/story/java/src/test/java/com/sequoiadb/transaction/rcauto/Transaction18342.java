package com.sequoiadb.transaction.rcauto;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18342 :: 版本: 1 :: 设置会话属性从备节点读取记录
 * @date 2019-5-17
 * @author luweikang
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18342 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl18342";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();
    private BSONObject data = null;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        cl = sdb.getCollectionSpace( csName ).createCollection( clName );
        data = ( BSONObject ) JSON
                .parse( "{_id: 1, a: 1, b: 'test session attr'}" );
        expList.add( data );
    }

    @Test
    public void test() {

        cl.insert( data );

        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "S" ) );

        DBCursor cursor = cl.query( null, null, "{'b': 1}", "{'':'a'}" );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

        // 表扫描记录
        cursor = cl.query( null, null, "{'b': 1}", "{'':null}" );
        actList = TransUtils.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        actList.clear();

    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

}
