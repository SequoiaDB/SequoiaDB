package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-33848:连接池第一个连接地址故障，获取连接
 * @Author liuli
 * @Date 2023.10.18
 * @UpdateAuthor liuli
 * @UpdateDate 2023.10.18
 * @version 1.10
 */
public class ConnectionPool33848 extends DataSourceTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb = null;
    private String noneUrl;
    private List< String > urlList = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        urlList = new ArrayList<>();
        noneUrl = SdbTestBase.hostName + ":" + SdbTestBase.reservedPortBegin;
        urlList.add( noneUrl );
        urlList.add( SdbTestBase.coordUrl );
    }

    // CI-2568:Linux下执行时受tcp超时时间影响，故屏蔽用例
    @Test(enabled = false)
    public void test() throws InterruptedException {
        ds = SequoiadbDatasource.builder().serverAddress( urlList ).build();
        // 获取当前时间
        long startTime = System.currentTimeMillis();

        // 获取连接
        sdb = ds.getConnection();

        // 获取结束时间
        long endTime = System.currentTimeMillis();

        // 获取连接时间超过200ms
        long timeDifference = endTime - startTime;
        long connectTimeout = 200;
        if ( timeDifference < connectTimeout ) {
            Assert.fail( "获取连接时间小于200ms,timeDifference:" + timeDifference
                    + ", url:" + sdb.getHost() + ":" + sdb.getPort() );
        }

        sdb.close();
        ds.close();

        // 设置获取连接超时时间为20ms
        ConfigOptions conf = new ConfigOptions();
        conf.setConnectTimeout( 20 );
        ds = SequoiadbDatasource.builder().serverAddress( urlList )
                .configOptions( conf ).build();

        // 获取当前时间
        startTime = System.currentTimeMillis();

        // 获取连接
        sdb = ds.getConnection();

        // 获取结束时间
        endTime = System.currentTimeMillis();

        // 获取连接时间超过200ms，预期getConnection时间大于20ms，小于200ms
        timeDifference = endTime - startTime;
        System.out.println( "获取连接时间:" + timeDifference );

    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
        if ( ds != null ) {
            ds.close();
        }
    }
}
