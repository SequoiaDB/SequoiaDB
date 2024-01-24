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
 * @Description seqDB-23836:配置MaxItemNum:-1，回收项目
 * @Author huangxiaoni
 * @Date 2021.07.02
 */
@Test(groups = "recycleBin")
public class RecycleBin23836 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBRecycleBin recycleBin;
    private Object maxItemNumOriVal;

    @BeforeClass
    private void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        recycleBin = sdb.getRecycleBin();
        // 获取原始值
        BasicBSONObject detail = ( BasicBSONObject ) recycleBin.getDetail();
        maxItemNumOriVal = detail.get( "MaxItemNum" );
    }

    @Test
    private void test() throws Exception {
        BasicBSONObject detail;
        // SdbRecycle.alter修改参数
        recycleBin.alter( new BasicBSONObject( "MaxItemNum", -1 ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "MaxItemNum" ), -1 );

        // 恢复初始值
        recycleBin
                .alter( new BasicBSONObject( "MaxItemNum", maxItemNumOriVal ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "MaxItemNum" ), maxItemNumOriVal );

        // SdbRecycle.setAttribute修改参数
        recycleBin.setAttributes( new BasicBSONObject( "MaxItemNum", -1 ) );
        detail = ( BasicBSONObject ) recycleBin.getDetail();
        Assert.assertEquals( detail.get( "MaxItemNum" ), -1 );
    }

    @AfterClass
    private void tearDown() {
        try {
            // 恢复初始值
            recycleBin.alter(
                    new BasicBSONObject( "MaxItemNum", maxItemNumOriVal ) );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }
}
