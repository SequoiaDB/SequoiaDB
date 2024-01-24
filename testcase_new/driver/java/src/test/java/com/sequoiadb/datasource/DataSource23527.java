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
 * @descreption seqDB-23527:最小闲置连接数验证
 * @author ZhangYanan
 * @date 2021/12/01
 * @updateUser ZhangYanan
 * @updateDate 2021/12/01
 * @updateRemark
 * @version 1.0
 */

public class DataSource23527 extends DataSourceTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private ObjectId lobID;
    private String clName = "cl_23527";
    private final int checkInterval = 5 * 1000;
    private int writeLobSize = 1024 * 1024 * 5;
    private int minIdleCount = 4;
    private int maxIdleCount = 8;

    @BeforeClass
    public void setUp() throws InterruptedException {
        ArrayList< String > addrs = new ArrayList<>();
        addrs.add( SdbTestBase.coordUrl );
        ConfigOptions nwOpt = new ConfigOptions();
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval( checkInterval / 20 );
        dsOpt.setMinIdleCount( minIdleCount );
        dsOpt.setMaxIdleCount( maxIdleCount );
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

    @Test()
    public void test() throws InterruptedException {
        // 获取连接执行读lob操作
        for ( int i = 0; i < maxIdleCount-1; i++ ) {
            Sequoiadb datasourceSdb1 = ds.getConnection();
            DBCollection cl1 = datasourceSdb1
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            try ( DBLob rLob = cl1.openLob( lobID, DBLob.SDB_LOB_READ )) {
                byte[] rbuff = new byte[ writeLobSize ];
                rLob.read( rbuff );
            }
            ds.releaseConnection( datasourceSdb1 );
        }
        Thread.sleep( checkInterval  );
        int actIdleCount = ds.getIdleConnNum();
        Assert.assertEquals( actIdleCount, ( minIdleCount + maxIdleCount ) / 2 );
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
