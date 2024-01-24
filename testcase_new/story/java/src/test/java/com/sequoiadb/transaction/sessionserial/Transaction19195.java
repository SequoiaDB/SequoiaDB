package com.sequoiadb.transaction.sessionserial;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 
 * @description seqDB-19195:修改事务配置不影响使用setSessionAtrr()设置过的会话
 * @author yinzhen
 * @date 2019年9月18日
 */
@Test(groups = "ru")
public class Transaction19195 extends SdbTestBase {

    private Sequoiadb sdb;
    private String clName = "cl_19195";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        sdb.getCollectionSpace( SdbTestBase.csName ).createCollection( clName );
    }

    @Test
    public void test() {
        Sequoiadb db1 = null;

        try {

            // 使用setSessionAttr设置事务属性，覆盖所有可设置的事务属性
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            BSONObject expAttr = ( BSONObject ) JSON.parse(
                    "{TransIsolation:1, TransTimeout:30, TransLockWait:true, TransAutoCommit:true, TransAutoRollback:false, TransUseRBS:false, TransRCCount:false}" );
            db1.setSessionAttr( expAttr );

            // 使用updateConf更新同样的事务属性为其他值
            db1.updateConfig( ( BSONObject ) JSON.parse(
                    "{transisolation:2, transactiontimeout:120, translockwait:false, transautocommit:false, transautorollback:true, transuserbs:true, transrccount:true}" ) );

            // 使用getSessionAttr检查事务属性
            BSONObject attr = db1.getSessionAttr();
            for ( String key : expAttr.keySet() ) {
                Assert.assertEquals( attr.get( key ), expAttr.get( key ) );
            }
        } finally {
            if ( null != db1 ) {
                db1.commit();
                db1.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( null != sdb ) {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
            sdb.deleteConfig( ( BSONObject ) JSON.parse(
                    "{transisolation:'', transactiontimeout:'', translockwait:'', transautocommit:'', transautorollback:'', transuserbs:'', transrccount:''}" ),
                    ( BSONObject ) JSON.parse( "{Global:true}" ) );
            sdb.close();
        }
    }
}
