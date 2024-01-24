package com.sequoiadb.recyclebin.serial;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBRecycleBin;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-23833:配置ExpireTime:-1，回收项目
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23833 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBRecycleBin recycleBin;
    private Object expireTimeOriVal;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        recycleBin = sdb.getRecycleBin();
        // 获取原始值
        BasicBSONObject detail = ( BasicBSONObject ) recycleBin.getDetail();
        expireTimeOriVal = detail.get( "ExpireTime" );
    }

    @Test
    private void test() throws Exception {
        BasicBSONObject detail;
        // SdbRecycle.alter修改参数
        recycleBin.alter( new BasicBSONObject( "ExpireTime", -1 ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "ExpireTime" ), -1 );

        // 恢复初始值
        recycleBin
                .alter( new BasicBSONObject( "ExpireTime", expireTimeOriVal ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "ExpireTime" ), expireTimeOriVal );

        // SdbRecycle.setAttribute修改参数
        recycleBin.setAttributes( new BasicBSONObject( "ExpireTime", -1 ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "ExpireTime" ), -1 );
    }

    @AfterClass
    private void tearDown() {
        try {
            // 恢复初始值
            recycleBin.alter(
                    new BasicBSONObject( "ExpireTime", expireTimeOriVal ) );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
