package com.sequoiadb.transaction.rc;

import java.util.List;

import org.bson.BSONObject;
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
 * @Description seqDB-18644:内置SQL支持隔离级别
 * @date 2019-7-9
 * @author yinzhen
 *
 */
@Test(groups = { "rc" })
public class Transaction18644 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String hashCLName = "cl18644_hash";
    private List< BSONObject > expList;
    private String hintIxScan = "{'':'idx18644'}";
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        // 创建分区表并插入记录R1s
        TransUtils.createHashCL( sdb, csName, hashCLName );
        cl = sdb.getCollectionSpace( csName ).getCollection( hashCLName );
        cl.createIndex( "idx18644", "{a:1}", false, false );
        expList = TransUtils.insertRandomDatas( cl, 0, 100 );
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
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( hashCLName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( hashCLName );

        // 事务1批量插入记录后为R2s
        TransUtils.insertRandomDatas( cl1, 100, 120 );

        // 事务2内置SQL查询
        DBCursor cursor = db2
                .exec( "select * from " + csName + "." + hashCLName );
        List< BSONObject > actList = TransUtils.getReadActList( cursor );
        TransUtils.sortList( actList );
        TransUtils.sortList( expList );
        Assert.assertEquals( actList, expList );
        cursor = db2.exec(
                "select count(a) as aCount from " + csName + "." + hashCLName );
        long countA = ( long ) cursor.getNext().get( "aCount" );
        Assert.assertEquals( countA, expList.size() );

        // 事务1批量更新记录为R3s
        cl1.update( null, "{$inc:{a:10}}", hintIxScan );

        // 事务2内置SQL查询
        cursor = db2.exec( "select * from " + csName + "." + hashCLName );
        actList = TransUtils.getReadActList( cursor );
        TransUtils.sortList( actList );
        Assert.assertEquals( actList, expList );
        cursor = db2.exec(
                "select count(a) as aCount from " + csName + "." + hashCLName );
        countA = ( long ) cursor.getNext().get( "aCount" );
        Assert.assertEquals( countA, expList.size() );

        // 事务1批量删除记录为R4s
        cl1.delete( "{$and:[{a:{$gte:0}}, {a:{$lt:30}}]}", hintIxScan );

        // 事务2内置SQL查询
        cursor = db2.exec( "select * from " + csName + "." + hashCLName );
        actList = TransUtils.getReadActList( cursor );
        TransUtils.sortList( actList );
        Assert.assertEquals( actList, expList );
        cursor = db2.exec(
                "select count(a) as aCount from " + csName + "." + hashCLName );
        countA = ( long ) cursor.getNext().get( "aCount" );
        Assert.assertEquals( countA, expList.size() );

        // 事务1提交
        db1.commit();

        // 事务2内置SQL查询
        List< BSONObject > expRecords = TransUtils.getIncDatas( 20, 120, 10 );
        cursor = db2.exec( "select * from " + csName + "." + hashCLName );
        actList = TransUtils.getReadActList( cursor );
        TransUtils.sortList( actList );
        TransUtils.sortList( expRecords );
        Assert.assertEquals( actList, expRecords );
        cursor = db2.exec(
                "select count(a) as aCount from " + csName + "." + hashCLName );
        countA = ( long ) cursor.getNext().get( "aCount" );
        Assert.assertEquals( countA, expRecords.size() );

        // 事务2提交
        db2.commit();
    }
}
