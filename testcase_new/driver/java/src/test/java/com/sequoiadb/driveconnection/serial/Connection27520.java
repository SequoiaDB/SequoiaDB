package com.sequoiadb.driveconnection.serial;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @descreption seqDB-27520:configOption( ConfigOption option)设置连接选项
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27520 extends SdbTestBase {
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27520";

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() throws Exception {
        // test a：不开启SSL,指定setUseSSL()
        ConfigOptions configOption = new ConfigOptions();
        configOption.setUseSSL( true );
        try {
            sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                    .userConfig( new UserConfig() )
                    .configOptions( configOption ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                    .getErrorCode() ) {
                throw e;
            }
        }

        // test b：开启SSL,指定setUseSSL()
        BSONObject sslConf = new BasicBSONObject();
        sslConf.put( "usessl", true );
        db.updateConfig( sslConf );
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( new UserConfig() ).configOptions( configOption )
                .build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：随机覆盖其他set接口
        configOption.setUseNagle( true );
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( new UserConfig() ).configOptions( configOption )
                .build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
    }

    @AfterClass
    public void tearDown() {
        BSONObject sslConf = new BasicBSONObject();
        sslConf.put( "usessl", 1 );
        BSONObject options = new BasicBSONObject();
        options.put( "Global", true );
        db.deleteConfig( sslConf, options );
        if ( db != null ) {
            db.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}