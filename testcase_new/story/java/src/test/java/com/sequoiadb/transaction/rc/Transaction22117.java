package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-22117:集合切分与其他集合事务交互
 * @date 2020-05-27
 * @author zhaoyu
 */
@Test(groups = "rc")
public class Transaction22117 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String clName = "cl22117_1";
    private String clName2 = "cl22117_2";
    private DBCollection cl1;
    private DBCollection cl2;
    private String srcGroup;
    private String desGroup;
    private List< BSONObject > expList1 = new ArrayList< BSONObject >();
    private List< BSONObject > expList2 = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
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
    }

    @Test
    public void testCommit() {
        cl1 = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + srcGroup + "'} " ) );
        cl2 = cs.createCollection( clName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'a':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        cl1.createIndex( "a", "{a:1}", false, false );

        expList1.clear();
        expList1 = TransUtils.insertRandomDatas( cl1, 0, 6 );
        expList2.clear();
        expList2 = TransUtils.getIncDatas( 0, 6, 1 );
        TransUtils.insertRandomDatas( cl2, 0, 6 );

        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl11 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl12 = db1.getCollectionSpace( csName )
                    .getCollection( clName2 );
            DBCollection cl21 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl22 = db2.getCollectionSpace( csName )
                    .getCollection( clName2 );

            // 开启2个事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );

            // 事务2更新集合1中的记录
            cl21.update( "", "{$inc:{a:1}}", null );

            // 执行切分,切分成功
            cl2.split( srcGroup, desGroup, 50 );

            // 事务1读集合1中的记录
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 事务2读集合1中的记录
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':'a'}",
                    expList2 );

            // 事务1读集合2中的记录
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 事务2读集合2中的记录
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 非事务中读记录
            TransUtils.queryAndCheck( cl1, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl2, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 提交事务
            db1.commit();

            // 事务1读集合1中的记录
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':'a'}",
                    expList2 );

            // 事务2读集合2中的记录
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 非事务中读记录
            TransUtils.queryAndCheck( cl1, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl2, null, "{_id:1}", "{'':'a'}",
                    expList1 );

        } finally {
            db1.rollback();
            db2.rollback();

            cs.dropCollection( clName );
            cs.dropCollection( clName2 );

            if ( db1 != null ) {
                db1.close();
            }

            if ( db2 != null ) {
                db2.close();
            }
        }
    }

    @Test
    public void testRollback() {
        cl1 = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + srcGroup + "'} " ) );
        cl2 = cs.createCollection( clName2,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'a':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        cl1.createIndex( "a", "{a:1}", false, false );

        expList1.clear();
        expList1 = TransUtils.insertRandomDatas( cl1, 0, 6 );
        expList2.clear();
        expList2 = TransUtils.getIncDatas( 0, 6, 1 );
        TransUtils.insertRandomDatas( cl2, 0, 6 );

        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl11 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl12 = db1.getCollectionSpace( csName )
                    .getCollection( clName2 );
            DBCollection cl21 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl22 = db2.getCollectionSpace( csName )
                    .getCollection( clName2 );

            // 开启2个事务
            db1.beginTransaction();
            db2.beginTransaction();

            // 事务1更新集合1中的记录
            cl11.update( "", "{$inc:{a:1}}", null );

            // 执行切分,切分成功
            cl2.split( srcGroup, desGroup, 50 );

            // 事务1读集合1中的记录
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl11, null, "{_id:1}", "{'':'a'}",
                    expList2 );

            // 事务2读集合1中的记录
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 事务1读集合2中的记录
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl12, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 事务2读集合2中的记录
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 非事务中读记录
            TransUtils.queryAndCheck( cl1, null, "{_id:1}", "{'':null}",
                    expList2 );
            TransUtils.queryAndCheck( cl2, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 回滚事务
            db1.rollback();

            // 事务2读集合1中的记录
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl21, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 事务2读集合2中的记录
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl22, null, "{_id:1}", "{'':'a'}",
                    expList1 );

            // 非事务中读记录
            TransUtils.queryAndCheck( cl1, null, "{_id:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl2, null, "{_id:1}", "{'':'a'}",
                    expList1 );

        } finally {
            db1.rollback();
            db2.rollback();

            cs.dropCollection( clName );
            cs.dropCollection( clName2 );

            if ( db1 != null ) {
                db1.close();
            }

            if ( db2 != null ) {
                db2.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

}
