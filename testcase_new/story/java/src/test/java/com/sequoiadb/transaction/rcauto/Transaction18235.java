package com.sequoiadb.transaction.rcauto;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-18235 :: 版本: 1 :: 失败不自动回滚只会显示开启事务生效
 * @date 2019-4-16
 * @author luweikang
 *
 */
@Test(groups = { "rcauto", "rrauto" })
public class Transaction18235 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1;
    private String clName = "cl18235";
    private DBCollection cl = null;
    private List< BSONObject > expList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        sdb1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less than two groups" );
        }

        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{a:1}, ShardingType:'hash', AutoSplit: true}" ) );

        cl.createIndex( "a", "{'a': 1, 'b': 1}", true, false );
    }

    @Test
    public void test() {
        DBCollection cl1 = sdb1.getCollectionSpace( csName )
                .getCollection( clName );

        // 开启事务1，插入记录
        TransUtils.beginTransaction( sdb1 );
        TransUtils.insertDatas( cl1, 0, 1000, 1 );
        cl1.update( "{a: 1}", "{$set: {a: 1000}}", null );
        cl1.delete( "{b:{$gte: 0, $lt: 500}}", null );
        List< BSONObject > datas1 = TransUtils.getUpdateDatas( 500, 1000, 1 );

        TransUtils.insertDatas( cl1, 1000, 2000, 2 );
        cl1.update( "{a: 2}", "{$set: {a: 2000}}", null );
        cl1.delete( "{b:{$gte: 1000, $lt: 1500}}", null );
        List< BSONObject > datas2 = TransUtils.getUpdateDatas( 1500, 2000, 2 );

        TransUtils.insertDatas( cl1, 2000, 3000, 3 );
        cl1.update( "{a: 3}", "{$set: {a: 3000}}", null );
        cl1.delete( "{b:{$gte: 2000, $lt: 2500}}", null );
        List< BSONObject > datas3 = TransUtils.getUpdateDatas( 2500, 3000, 3 );

        try {
            cl1.update( "{a: 1}", "{$set: {b: 10000}}", "{'': null}" );
            Assert.fail(
                    "update records as duplicate records should be error" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
        }

        TransUtils.insertDatas( cl1, 3000, 4000, 4 );
        cl1.update( "{a: 4}", "{$set: {a: 4000}}", null );
        cl1.delete( "{b:{$gte: 3000, $lt: 3500}}", null );
        List< BSONObject > datas4 = TransUtils.getUpdateDatas( 3500, 4000, 4 );

        TransUtils.insertDatas( cl1, 4000, 5000, 5 );
        cl1.update( "{a: 5}", "{$set: {a: 5000}}", null );
        cl1.delete( "{b:{$gte: 4000, $lt: 4500}}", null );
        List< BSONObject > datas5 = TransUtils.getUpdateDatas( 4500, 5000, 5 );

        TransUtils.commitTransaction( sdb1 );

        // 索引扫描记录
        expList.clear();
        expList.addAll( datas1 );
        expList.addAll( datas2 );
        expList.addAll( datas3 );
        expList.addAll( datas4 );
        expList.addAll( datas5 );

        // update不是原子操作,所以更新多条记录为相同记录时,第一条记录是更新成功的,第二条失败,
        // 所以根据排序将更新成功的第一条记录手动修改并排放到预期记录末尾
        expList.remove( 0 );
        expList.add( ( BSONObject ) JSON
                .parse( "{'_id': 500, 'a': 1, 'b': 10000}" ) );
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
        sdb1.commit();
        if ( !sdb1.isClosed() ) {
            sdb1.close();
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
