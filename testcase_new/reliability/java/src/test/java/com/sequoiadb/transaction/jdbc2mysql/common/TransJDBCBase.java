package com.sequoiadb.transaction.jdbc2mysql.common;

import java.sql.SQLException;
import java.util.List;

import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

public class TransJDBCBase extends SdbTestBase {
    protected Sequoiadb sdb;
    private String clName;
    private int insertNum;

    protected void initCL( String clName, int insertNum ) {
        this.clName = clName;
        this.insertNum = insertNum;
    }

    public void setSdb( Sequoiadb sdb ) {
        this.sdb = sdb;
    }

    protected int getInsertNum() {
        return insertNum;
    }

    private void init() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    protected void beforeSetUp() throws ReliabilityException {
    }

    protected void afterSetUp() throws ReliabilityException {
    }

    protected void beforeTearDown() {
    }

    protected void afterTearDown() {
    }

    @BeforeClass
    public void setUp() throws Exception {
        init();

        beforeSetUp();

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "ONE GROUP MODE" );
        }
        GroupMgr groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "GROUP ERROR" );
        }

        // MySQL 创建表(balance, account) 并插入数据 insertNum 个账户，每个账户 10000 元
        TransferJDBCTh.initTrans( clName, insertNum );

        afterSetUp();
    }

    @AfterClass
    public void tearDown() throws InterruptedException, SQLException {
        try {
            beforeTearDown();

            TransferJDBCTh.finiTrans( clName );

            afterTearDown();
        } finally {
            fini();
        }
    }

    private void fini() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
