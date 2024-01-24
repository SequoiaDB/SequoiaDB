package com.sequoiadb.transaction.rc;

import java.util.List;

import org.bson.BSONObject;
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
 * @Description seqDB-18637:批量插入/更新/删除记录与带查询条件执行查询并发，事务回滚
 * @date 2019-7-9
 * @author yinzhen
 *
 */
@Test(groups = { "rc" })
public class Transaction18637 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String mainCLName = "cl18637_main";
    private List< BSONObject > expList;
    private String hintTbScan = "{'':null}";
    private String hintIxScan = "{'':'idx18637'}";
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

        // 创建主子表并插入记录R1s
        TransUtils.createMainCL( sdb, csName, mainCLName, "subcl18637_1",
                "subcl18637_2", 3000 );
        cl = sdb.getCollectionSpace( csName ).getCollection( mainCLName );
        cl.createIndex( "idx18637", "{a:1}", false, false );
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
            cs.dropCollection( mainCLName );
            sdb.close();
        }
    }

    @Test
    public void test() {
        // 开启两个并发事务
        TransUtils.beginTransaction( db1 );
        TransUtils.beginTransaction( db2 );
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( mainCLName );
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( mainCLName );

        // 事务1批量插入/更新/删除记录后为R2s
        TransUtils.insertRandomDatas( cl1, 100, 120 );
        cl1.update( "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}", "{$inc:{a:10}}",
                hintIxScan );
        cl1.delete( "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}", hintIxScan );

        // 事务1回滚
        db1.rollback();

        // 事务2表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl2, "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}",
                null, "{a:1}", hintTbScan, expList );
        TransUtils.queryAndCheck( cl2, "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}",
                null, "{a:1}", hintIxScan, expList );

        // 非事务表扫描/索引扫描记录
        TransUtils.queryAndCheck( cl, "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}",
                null, "{a:1}", hintTbScan, expList );
        TransUtils.queryAndCheck( cl, "{$and:[{a:{$gte:0}}, {a:{$lt:120}}]}",
                null, "{a:1}", hintIxScan, expList );

        // 事务2提交
        db2.commit();
    }
}
