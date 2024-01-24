package com.sequoiadb.transaction.ru;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
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
 * @FileName:seqDB-16147:数据组节点开启事务，并发执行非事务操作（bulkinsert和truncate并发） 插入数据，一条线程执行insert，另一条线程执行truncate
 * @Author wangkexin
 * @Date 2018-09-18
 * @Version 1.00
 */
@Test(groups = "ru")
public class Transaction16147 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl16147";
    private CollectionSpace cs = null;
    private int insertNum = 100000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        DBCollection cl = cs.createCollection( clName );
        TransUtils.insertRandomDatas( cl, 0, insertNum );
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        InsertThread insertThread = new InsertThread();

        truncateThread.start();
        insertThread.start( 1 );

        Assert.assertTrue( truncateThread.isSuccess(),
                truncateThread.getErrorMsg() );

        Assert.assertTrue( insertThread.isSuccess(),
                insertThread.getErrorMsg() );

    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_TRUNCATED
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode() ) {
                    e.printStackTrace();
                    throw e;
                }
            }
        }
    }

    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 10; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse(
                        "{sk:" + value + ", test:" + "'insert_test'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }
}
