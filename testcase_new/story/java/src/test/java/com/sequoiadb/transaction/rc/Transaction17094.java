package com.sequoiadb.transaction.rc;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @FileName:seqDB-17094：插入与更新并发，事务回滚，过程中读 更新走索引扫描
 * @Author zhaoyu
 * @Date 2019-01-16
 * @Version 1.00
 */
@Test(groups = { "rc", "rr" })
public class Transaction17094 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private Sequoiadb db3;
    private DBCollection cl = null;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private String hintTbScan = "{\"\":null}";
    private String hintIxScan = "{\"\":\"a\"}";
    private String orderByPos = "{a:1}";
    private String orderByRev = "{a: -1}";
    private int startId = 0;
    private int stopId = 1000;
    private int updateValue = 20000;
    private String hashCLName = "cl17094_hash";
    private String mainCLName = "cl17094_main";
    private String subCLName1 = "subcl17094_1";
    private String subCLName2 = "subcl17094_2";

    @DataProvider(name = "index")
    public Object[][] createIndex() {
        return new Object[][] { { "{'a': 1}", hashCLName },
                { "{'a': -1, 'b': 1}", mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        TransUtils.createCLs( sdb, csName, hashCLName, mainCLName, subCLName1,
                subCLName2, 500 );
    }

    @AfterClass
    public void tearDown() {
        // 关闭所有游标
        sdb.closeAllCursors();
        db1.closeAllCursors();
        db2.closeAllCursors();
        db3.closeAllCursors();

        // 先关闭事务连接，再删除集合
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        if ( !db1.isClosed() ) {
            db1.close();
        }
        if ( !db2.isClosed() ) {
            db2.close();
        }
        if ( !db3.isClosed() ) {
            db3.close();
        }
        if ( cs.isCollectionExist( hashCLName ) ) {
            cs.dropCollection( hashCLName );
        }
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    @Test(dataProvider = "index")
    public void test( String indexKey, String clName )
            throws InterruptedException {
        try {
            cl = sdb.getCollectionSpace( csName ).getCollection( clName );
            cl.createIndex( "a", indexKey, false, false );

            // 1 开启3个并发事务
            TransUtils.beginTransaction( db1 );
            TransUtils.beginTransaction( db2 );
            TransUtils.beginTransaction( db3 );
            cl1 = db1.getCollectionSpace( csName ).getCollection( clName );
            cl2 = db2.getCollectionSpace( csName ).getCollection( clName );
            cl3 = db3.getCollectionSpace( csName ).getCollection( clName );

            // 2 事务1插入记录R1
            TransUtils.insertRandomDatas( cl1, startId, stopId );

            // 3 事务2匹配记录R1更新为R2
            UpdateThread updateThread = new UpdateThread();
            updateThread.start();
            Assert.assertTrue( TransUtils.isTransWaitLock( sdb,
                    updateThread.getTransactionID() ) );

            // 7 回滚事务1
            db1.rollback();
            Assert.assertTrue( updateThread.isSuccess(),
                    updateThread.getErrorMsg() );

            // 7 非事务记录读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintTbScan, new ArrayList< BSONObject >() );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintIxScan, new ArrayList< BSONObject >() );

            // 7 非事务记录逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintTbScan, new ArrayList< BSONObject >() );

            // 非事务索引逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintIxScan, new ArrayList< BSONObject >() );

            // 8 事务2记录读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintTbScan, new ArrayList< BSONObject >() );

            // 事务2索引读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintIxScan, new ArrayList< BSONObject >() );

            // 8 事务2记录逆序读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintTbScan, new ArrayList< BSONObject >() );

            // 事务2索引逆序读
            TransUtils.queryAndCheck( cl2,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintIxScan, new ArrayList< BSONObject >() );

            // 9 事务3记录读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintTbScan, new ArrayList< BSONObject >() );

            // 事务3索引读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintIxScan, new ArrayList< BSONObject >() );

            // 9 事务3记录逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintTbScan, new ArrayList< BSONObject >() );

            // 事务3索引逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintIxScan, new ArrayList< BSONObject >() );

            // 10 提交事务2
            db2.commit();

            // 10 非事务记录读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintTbScan, new ArrayList< BSONObject >() );

            // 非事务索引读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintIxScan, new ArrayList< BSONObject >() );

            // 10 非事务记录逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintTbScan, new ArrayList< BSONObject >() );

            // 非事务索引逆序读
            TransUtils.queryAndCheck( cl,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintIxScan, new ArrayList< BSONObject >() );

            // 11 事务3记录读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintTbScan, new ArrayList< BSONObject >() );

            // 事务3索引读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByPos, hintIxScan, new ArrayList< BSONObject >() );

            // 11 事务3记录逆序读
            TransUtils.queryAndCheck( cl3,
                    "{a:{$lt:" + ( stopId + updateValue ) + ",$gte:" + startId
                            + "}}",
                    orderByRev, hintTbScan, new ArrayList< BSONObject >() );

            // 事务3索引逆序读
            TransUtils.queryAndCheck( cl3, orderByRev, hintIxScan,
                    new ArrayList< BSONObject >() );

            // 提交事务3
            db3.commit();
        } finally {
            if ( cl.isIndexExist( "a" ) ) {
                cl.dropIndex( "a" );
            }
            cl.truncate();
        }
    }

    private class UpdateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            // 判断事务阻塞需先获取事务id
            setTransactionID( cl2.getSequoiadb() );

            // 更新走索引
            cl2.update( null, "{$inc:{a:" + updateValue + "}}", hintIxScan );
        }
    }

}
