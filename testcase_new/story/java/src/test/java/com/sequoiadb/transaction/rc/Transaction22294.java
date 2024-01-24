package com.sequoiadb.transaction.rc;

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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22294:切分与query for update/query and update/query and remove并发
 * @date 2020-06-15
 * @author zhaoyu
 * 
 */
@Test(groups = { "rc", "rr" })
public class Transaction22294 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String clName = "cl22294";
    private DBCollection cl = null;
    private CollectionSpace cs;
    private String srcGroup;
    private String desGroup;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws InterruptedException {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone!" );
        }
        List< String > groupsNames = CommLib.getDataGroupNames( sdb );
        if ( groupsNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroup = groupsNames.get( 0 );
        desGroup = groupsNames.get( 1 );
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'b':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        cl.createIndex( "a", "{a:1}", false, false );
        expList = TransUtils.insertRandomDatas( cl, 0, 6 );
    }

    @Test
    public void test() throws Exception {
        Sequoiadb db1 = null;
        try {
            // 开启读事务T1
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            db1.beginTransaction();

            // 其他连接上执行切分
            cl.split( srcGroup, desGroup, 50 );

            // 事务中执行query for update
            DBCursor cursor = cl1.query( "", "", "{a:1}", "{'':'a'}",
                    DBQuery.FLG_QUERY_FOR_UPDATE );
            ArrayList< BSONObject > actList = TransUtils
                    .getReadActList( cursor );
            Assert.assertEquals( actList, expList );

            // 事务中执行query and update
            Thread.sleep( 1000 );
            List< BSONObject > expList1 = TransUtils.getIncDatas( 0, 6, 1 );
            cursor = cl1
                    .queryAndUpdate( null, null, new BasicBSONObject( "a", 1 ),
                            null,
                            new BasicBSONObject( "$inc",
                                    new BasicBSONObject( "a", 1 ) ),
                            0, -1, 0, false );
            while ( cursor.hasNext() ) {
                cursor.getNext();
            }
            cursor.close();

            // 查询
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':null}", expList1 );

            // 事务中执行query and remove
            cursor = cl1.queryAndRemove( null, null,
                    ( BSONObject ) JSON.parse( "{a:1}" ),
                    ( BSONObject ) JSON.parse( "{'':'a'}" ), 0, -1, 0 );
            while ( cursor.hasNext() ) {
                cursor.getNext();
            }
            cursor.close();

            // 查询
            TransUtils.queryAndCheck( cl, "{a:1}", "{'':'a'}",
                    new ArrayList< BSONObject >() );

        } finally {
            if ( db1 != null && !db1.isClosed() ) {
                db1.rollback();
                db1.close();
            }

        }

    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }
}
