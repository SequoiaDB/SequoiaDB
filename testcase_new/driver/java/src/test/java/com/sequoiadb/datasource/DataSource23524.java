package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @descreption seqDB-23524:recv buffer大于CacheLimit的连接回池
 * @author ZhangYanan
 * @date 2021/12/01
 * @updateUser ZhangYanan
 * @updateDate 2021/12/01
 * @updateRemark
 * @version 1.0
 */

public class DataSource23524 extends DataSourceTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb;
    private DBCollection cl;
    private CollectionSpace cs;
    private ObjectId lobID;
    private String clName = "cl_23524";
    private final int checkInterval = 5 * 1000;
    private int writeLobSize = 1024 * 1024 * 2;

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
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        byte[] lobBuff = getRandomBytes( writeLobSize );
        try ( DBLob lob = cl.createLob()) {
            lob.write( lobBuff );
            lobID = lob.getID();
        }
    }

    @Test
    public void test() throws InterruptedException {
        // 初次获取连接
        Sequoiadb datasourceSdb = ds.getConnection();
        int firstHashCode = datasourceSdb.hashCode();
        DBCollection cl = datasourceSdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        try ( DBLob rLob = cl.openLob( lobID, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ writeLobSize ];
            rLob.read( rbuff );
        }
        ds.releaseConnection( datasourceSdb );

        // 释放后再次获取
        datasourceSdb = ds.getConnection();
        int secondHashCode = datasourceSdb.hashCode();
        Assert.assertNotEquals( firstHashCode, secondHashCode,
                "these two hashCodes should not be equal ! firstHashCode ="
                        + firstHashCode + ", secondHashCode ="
                        + secondHashCode );
        ds.releaseConnection( datasourceSdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) )
                cs.dropCollection( clName );
        } finally {
            sdb.close();
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
