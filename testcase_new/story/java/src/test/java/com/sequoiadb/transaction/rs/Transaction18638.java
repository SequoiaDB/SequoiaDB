package com.sequoiadb.transaction.rs;

import java.util.List;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @Description seqDB-18638:批量插入记录与查询并发，事务提交
 * @date 2019-7-9
 * @author yinzhen
 *
 */
@Test(groups = { "rs" })
public class Transaction18638 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private String hashCLName = "cl18638_hash";
    private List< BSONObject > expList;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'idx18638'}";
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }

        // 创建分区表
        TransUtils.createHashCL( sdb, csName, hashCLName );
        cl = sdb.getCollectionSpace( csName ).getCollection( hashCLName );
        cl.createIndex( "idx18638", "{a:1}", false, false );
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
        if ( db3 != null ) {
            db3.commit();
            db3.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( hashCLName );
            sdb.close();
        }
    }

    @Test
    public void test() throws InterruptedException {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db3 );
        TransUtils.beginTransaction( db2 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( hashCLName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( hashCLName );
        DBCollection cl3 = db3.getCollectionSpace( csName )
                .getCollection( hashCLName );

        // 事务1批量插入记录后为R1s
        expList = TransUtils.insertRandomDatas( cl1, 0, 10000 );

        // 事务2表扫描/索引扫描记录
        Query th2_1 = new Query( cl2, hintTbScan, expList );
        th2_1.start();

        Query th2_2 = new Query( cl3, hintIxScan, expList );
        th2_2.start();

        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, th2_1.getTransactionID() ) );
        Assert.assertTrue(
                TransUtils.isTransWaitLock( sdb, th2_2.getTransactionID() ) );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan, expList );
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan, expList );

        // 事务1提交
        db1.commit();

        Assert.assertTrue( th2_1.isSuccess(), th2_1.getErrorMsg() );
        Assert.assertTrue( th2_2.isSuccess(), th2_2.getErrorMsg() );

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{a:1}", hintTbScan, expList );
        TransUtils.queryAndCheck( cl2, "{a:1}", hintIxScan, expList );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{a:1}", hintTbScan, expList );
        TransUtils.queryAndCheck( cl, "{a:1}", hintIxScan, expList );

        // 事务2提交
        db2.commit();
        db3.commit();
    }

    private class Query extends SdbThreadBase {
        private String hint;
        private List< BSONObject > expList;
        private DBCollection cl;

        private Query( DBCollection cl, String hint,
                List< BSONObject > expList ) {
            this.cl = cl;
            this.hint = hint;
            this.expList = expList;
        }

        @Override
        public void exec() throws Exception {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl.getSequoiadb() );

            TransUtils.queryAndCheck( cl, "{a:1}", hint, expList );
        }
    }
}
