package com.sequoiadb.ssl;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.basicoperation.Commlib;
import com.sequoiadb.testcommon.SdbTestBase;


/**
 * @description  seqDB-11314:set/getUseSSL()接口测试
 * @author wuyan
 * @date 2021.5.13
 * @updateUser wuyan
 * @updateDate 2022.05.26
 * @updateRemark 修改配置方式改为使用updateConf接口
 * @version 1.10
 */

public class TestSSL11314 extends SdbTestBase {
    private String clName = "cl_11314";
    private static Sequoiadb sdb = null;
    private static Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( Commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        BSONObject sslConf = new BasicBSONObject();
        sslConf.put( "usessl", true );
        sdb.updateConfig( sslConf );
    }

    @Test
    public void testSSL() {
        // not open ssl
        ConfigOptions options = new ConfigOptions();
        Assert.assertEquals( options.getUseSSL(), false, "not open ssl" );

        // open ssl
        options.setUseSSL( true );
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );

        // test createcl
        cs = db.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );

        // test insert
        String value = "{a:1}";
        cl.insert( value );
        Assert.assertEquals( 1, cl.getCount( value ), "insert data error" );

        // test getSSL()
        boolean flag = options.getUseSSL();
        Assert.assertEquals( flag, true, "ssl should open" );

    }

    @AfterClass()
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

        } finally {
            BSONObject sslConf = new BasicBSONObject();
            sslConf.put( "usessl", 1 );
            BSONObject options = new BasicBSONObject();
            options.put("Global",true);
            sdb.deleteConfig( sslConf, options );
            if ( db != null ) {
                db.close();
            }
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
