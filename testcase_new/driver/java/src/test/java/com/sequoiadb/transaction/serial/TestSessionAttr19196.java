package com.sequoiadb.transaction.serial;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description: seqDB-19196 无事务操作时，获取会话缓存中的事务配置
 * @Author wangkexin
 * @Date 2019.09.04
 */

public class TestSessionAttr19196 extends SdbTestBase {
    private Sequoiadb db;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() {
        BSONObject sessionAttr = db.getSessionAttr();
        // check the default transaction attribute of session
        Assert.assertEquals( sessionAttr.get( "TransIsolation" ), 0 );
        Assert.assertEquals( sessionAttr.get( "TransTimeout" ), 60 );
        Assert.assertEquals( sessionAttr.get( "TransUseRBS" ), true );
        Assert.assertEquals( sessionAttr.get( "TransLockWait" ), false );
        Assert.assertEquals( sessionAttr.get( "TransAutoCommit" ), false );
        Assert.assertEquals( sessionAttr.get( "TransAutoRollback" ), true );
        Assert.assertEquals( sessionAttr.get( "TransRCCount" ), true );

        // set TransIsolation to RS
        db.setSessionAttr( new BasicBSONObject( "TransIsolation", 2) );
        sessionAttr.put( "TransIsolation", 2 );
        checkSessionAttr( sessionAttr , true);

        BSONObject updateConfig = new BasicBSONObject();
        updateConfig.put( "transisolation", 0 );
        updateConfig.put( "transactiontimeout", 120 );
        try {
            // set TransIsolation to RU, set transactiontimeout to 120s
            db.updateConfig( updateConfig );
            checkSessionAttr( sessionAttr, true );

            // check again
            sessionAttr.put( "TransTimeout", 120 );
            checkSessionAttr( sessionAttr, false );
        }finally {
            updateConfig.put( "transactiontimeout", 60 );
            db.updateConfig( updateConfig );
        }
    }

    @AfterClass
    public void teardown() {
        db.close();
    }

    private void checkSessionAttr( BSONObject expSessionAttr, boolean useCache ) {
        BSONObject result = db.getSessionAttr( useCache );
        Assert.assertEquals( result.toString(), expSessionAttr.toString() );
    }
}
