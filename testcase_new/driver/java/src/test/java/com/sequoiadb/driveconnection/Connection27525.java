package com.sequoiadb.driveconnection;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @descreption seqDB-27525:SequoiadbDatasource()方式设置datasourceOptions(DataOptions
 *              option)
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27525 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27525";

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void test() throws Exception {
        DatasourceOptions datasourceOption = new DatasourceOptions();
        ConnectStrategy strategy = ConnectStrategy.RANDOM;
        datasourceOption.setConnectStrategy( strategy );
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( new UserConfig() )
                .configOptions( new ConfigOptions() )
                .datasourceOptions( datasourceOption ).build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        Assert.assertEquals( strategy,
                ds.getDatasourceOptions().getConnectStrategy(),
                "The expected result are equal" );
    }

    @AfterClass
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}