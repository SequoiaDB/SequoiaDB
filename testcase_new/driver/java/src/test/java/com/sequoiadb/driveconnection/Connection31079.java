package com.sequoiadb.driveconnection;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-31079 : 获取连接超时
 * @Author Cheng Jingjing
 * @CreateDate 2023/4/11
 * @UpdateUser
 * @UpdateDate 2023/4/11
 * @UpdateRemark
 * @Version v1.0
 */
public class Connection31079 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() throws Exception {
        String address = SdbTestBase.hostName + ":" + 10;
        // case 1: connTime = 0
        testTimeOut( address, 2000, 0, 2000,
                SDBError.SDB_TIMEOUT.getErrorCode() );

        // case 2: connTime < retryTime
        testTimeOut( address, 3000, 1000, 3000,
                SDBError.SDB_TIMEOUT.getErrorCode() );

        // case 3: timeout = 0
        testTimeOut( address, 0, 2000, 2000,
                SDBError.SDB_NETWORK.getErrorCode() );

        // case 4: timeout < connTime
        testTimeOut( address, 1000, 2000, 3000,
                SDBError.SDB_TIMEOUT.getErrorCode() );
    }
    public void testTimeOut( String address, int timeout, int connTime,
                             int retryTime, int errorCode ) throws InterruptedException {
        int deviationTime = 1000;
        ConfigOptions netOpt = new ConfigOptions();
        netOpt.setConnectTimeout( connTime );
        netOpt.setMaxAutoConnectRetryTime( retryTime );

        SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress( address ).configOptions( netOpt ).build();

        long startTime = System.currentTimeMillis();
        try {
            sdb = ds.getConnection( timeout );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != errorCode ) {
                throw e;
            }
        } finally {
            long endTime = System.currentTimeMillis();
            long time = startTime - endTime;
            if ( timeout > 0 && ( time > ( timeout + deviationTime ) ) ) {
                Assert.fail( "Inaccurate time, except time: " + timeout
                        + " actual time: " + time );
            }
        }
        ds.close();
    }

    @AfterClass
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
        if ( db != null ) {
            db.close();
        }
        if ( sdb != null ) {
            db.close();
        }
    }
}