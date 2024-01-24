package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.Random;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @descreption seqDB-23525:sendbuffer小于等于CacheLimit的连接回池
 * @author ZhangYanan
 * @date 2021/12/01
 * @updateUser ZhangYanan
 * @updateDate 2021/12/01
 * @updateRemark
 * @version 1.0
 */

public class DataSource23525 extends DataSourceTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private String clName = "cl_23525";
    private final int checkInterval = 5 * 1000;
    private int writeLobSize = 1024 * 100;

    @BeforeClass
    public void setUp() throws InterruptedException {
        ArrayList< String > addrs = new ArrayList<>();
        addrs.add( SdbTestBase.coordUrl );
        ConfigOptions nwOpt = new ConfigOptions();
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval( checkInterval / 2 );
        dsOpt.setMinIdleCount( 1 );
        dsOpt.setMaxIdleCount( 1 );
        ds = new SequoiadbDatasource( addrs, "", "", nwOpt, dsOpt );
        Thread.sleep( checkInterval );
    }

    @Test
    public void test() throws InterruptedException {
        // 初次获取连接
        sdb = ds.getConnection();
        int firstHashCode = sdb.hashCode();
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl = cs.createCollection( clName );
        byte[] lobBuff = getRandomBytes( writeLobSize );
        try ( DBLob lob = cl.createLob()) {
            lob.write( lobBuff );
        }
        ds.releaseConnection( sdb );

        // 释放后再次获取
        sdb = ds.getConnection();
        int secondHashCode = sdb.hashCode();
        Assert.assertEquals( firstHashCode, secondHashCode );
        ds.releaseConnection( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) )
                cs.dropCollection( clName );
        } finally {
            ds.close();
        }
    }

    public byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }
}
