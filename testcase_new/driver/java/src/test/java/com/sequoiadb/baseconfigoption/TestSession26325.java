package com.sequoiadb.baseconfigoption;

import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import java.util.ArrayList;
import java.util.List;

/**
 * @descreption seqDB-26325:Prefered更正为Preferred验证
 * @author ZhangYanan
 * @date 2022/04/05
 * @updateUser ZhangYanan
 * @updateDate 2022/04/05
 * @updateRemark
 * @version 1.0
 */
public class TestSession26325 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private SequoiadbDatasource ds1 = null;
    private SequoiadbDatasource ds2 = null;

    @BeforeClass
    public void setSdb() {
        // 建立 SequoiaDB 数据库连接
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone" );
        }
    }

    @Test
    public void test() throws InterruptedException {
        String preferred = "PreferredInstance";
        sdb.setSessionAttr( new BasicBSONObject( preferred, "-S" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferred ), "-S" );

        String preferredMode = "PreferredInstanceMode";
        sdb.setSessionAttr( new BasicBSONObject( preferredMode, "ordered" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferredMode ),
                "ordered" );

        preferred = "PreferedInstance";
        sdb.setSessionAttr( new BasicBSONObject( preferred, "M" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferred ), "M" );

        preferredMode = "PreferedInstanceMode";
        sdb.setSessionAttr( new BasicBSONObject( preferredMode, "random" ) );
        Assert.assertEquals( sdb.getSessionAttr().get( preferredMode ),
                "random" );

        // 验证连接池新接口
        DatasourceOptions dsOpt1 = new DatasourceOptions();
        List< String > preferredInstance = new ArrayList<>();
        preferredInstance.add( "S" );
        dsOpt1.setPreferredInstance( preferredInstance );
        dsOpt1.setPreferredInstanceMode( "ordered" );
        ds1 = new SequoiadbDatasource( SdbTestBase.coordUrl, "", "", dsOpt1 );
        Assert.assertEquals( ds1.getDatasourceOptions().getPreferredInstance(),
                preferredInstance );
        Assert.assertEquals(
                ds1.getDatasourceOptions().getPreferredInstanceMode(),
                "ordered" );

        // 验证连接池废弃接口
        DatasourceOptions dsOpt2 = new DatasourceOptions();
        dsOpt1.setPreferedInstance( preferredInstance );
        dsOpt1.setPreferedInstanceMode( "ordered" );
        ds2 = new SequoiadbDatasource( SdbTestBase.coordUrl, "", "", dsOpt2 );
        Assert.assertEquals( ds1.getDatasourceOptions().getPreferedInstance(),
                preferredInstance );
        Assert.assertEquals(
                ds1.getDatasourceOptions().getPreferedInstanceMode(),
                "ordered" );
    }

    @AfterClass
    public void closeSdb() {
        if ( sdb != null ) {
            sdb.close();
        }
        if ( ds1 != null ) {
            ds1.close();
        }
        if ( ds2 != null ) {
            ds2.close();
        }
    }
}