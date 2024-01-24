package com.sequoiadb.baseconfigoption;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-23522:PreferedInstance对大小写不敏感
 * @Author Yipan
 * @Date 2021/02/02
 */
public class TestSession23522 extends SdbTestBase {
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setSdb() {
        // 建立 SequoiaDB 数据库连接
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone" );
        }
    }

    @Test
    public void test() {
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-S" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-S" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-M" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-M" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-A" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-A" );

        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-s" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-S" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-m" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-M" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "-a" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "-A" );

        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "S" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "S" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "M" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "M" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "A" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "A" );

        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "s" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "S" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "m" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "M" );
        sdb.setSessionAttr( new BasicBSONObject( "PreferedInstance", "a" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( "PreferedInstance" ),
                "A" );
    }

    @AfterClass
    public void closeSdb() {
        sdb.close();
    }

}