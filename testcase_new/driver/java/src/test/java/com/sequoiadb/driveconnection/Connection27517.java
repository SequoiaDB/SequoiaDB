package com.sequoiadb.driveconnection;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @descreption seqDB-27517:serverAddress(String address)设置节点地址
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27517 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_27517";

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void test() throws Exception {
        // test a：指定可用地址
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        // 关闭连接后执行操作
        sdb.close();
        try {
            sdb.createCollectionSpace( csName );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // test b：多次调用,指定最后一次为可用地址
        String wrongUrl = SdbTestBase.hostName + ":" + "10";
        sdb = Sequoiadb.builder().serverAddress( wrongUrl )
                .serverAddress( SdbTestBase.coordUrl ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：指定地址为null、""
        wrongUrl = null;
        try {
            sdb = Sequoiadb.builder().serverAddress( wrongUrl ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }
        wrongUrl = "";
        try {
            sdb = Sequoiadb.builder().serverAddress( wrongUrl ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // test d：指定不可用地址
        wrongUrl = "hostName" + ":" + "30";
        try {
            sdb = Sequoiadb.builder().serverAddress( wrongUrl ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                    .getErrorCode() ) {
                throw e;
            }
        }

        // test e：多次调用,指定最后一次为不可用地址
        try {
            sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                    .serverAddress( wrongUrl ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}