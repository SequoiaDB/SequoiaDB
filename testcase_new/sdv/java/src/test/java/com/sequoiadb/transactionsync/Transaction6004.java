package com.sequoiadb.transactionsync;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-6004:配置事务锁超时时间值非法校验_SD.transaction.015(设置事务锁等待超时时间值为-1,a01,3601)
 * @author wangkexin
 * @date 2019.04.08
 * @review
 */

public class Transaction6004 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private BSONObject options = new BasicBSONObject();

    @BeforeClass
    private void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
    }

    @Test
    public void test() throws Exception {
        List< String > dataGroupNames = CommLib.getDataGroupNames( sdb );
        List< String > nodeList = CommLib.getNodeAddress( sdb,
                dataGroupNames.get( 0 ) );
        String NodeName = nodeList.get( 0 );

        // test transactiontimeout is -1
        BSONObject configs1 = new BasicBSONObject();
        configs1.put( "transactiontimeout", -1 );
        options.put( "NodeName", NodeName );
        try {
            sdb.updateConfig( configs1, options );
            Assert.fail( "exp failed but succ." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        BSONObject selector = new BasicBSONObject();
        selector.put( "transactiontimeout", 1 );

        DBCursor cursor1 = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, options,
                selector, null );
        int expTransTimeout = 60;
        while ( cursor1.hasNext() ) {
            int actValue = ( int ) cursor1.getNext()
                    .get( "transactiontimeout" );
            Assert.assertEquals( actValue, expTransTimeout );
        }

        // test transactiontimeout is a01
        BSONObject configs2 = new BasicBSONObject();
        configs2.put( "transactiontimeout", "a01" );

        try {
            sdb.updateConfig( configs2, options );
            Assert.fail( "exp failed but succ." );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_INVALIDARG.getErrorCode() );
        }

        DBCursor cursor2 = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, options,
                selector, null );
        int expTransTimeout2 = 60;
        while ( cursor2.hasNext() ) {
            int actValue = ( int ) cursor2.getNext()
                    .get( "transactiontimeout" );
            Assert.assertEquals( actValue, expTransTimeout2 );
        }

        // test transactiontimeout is 3601
        BSONObject configs3 = new BasicBSONObject();
        configs3.put( "transactiontimeout", 3601 );
        sdb.updateConfig( configs3, options );

        DBCursor cursor3 = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, options,
                selector, null );
        int expTransTimeout3 = 3600;
        while ( cursor3.hasNext() ) {
            int actValue = ( int ) cursor3.getNext()
                    .get( "transactiontimeout" );
            Assert.assertEquals( actValue, expTransTimeout3 );
        }
    }

    @AfterClass
    private void teardown() {
        try {
            // 恢复环境
            BSONObject configs4 = new BasicBSONObject();
            configs4.put( "transactiontimeout", 60 );
            sdb.updateConfig( configs4, options );
        } finally {
            sdb.close();
        }
    }
}