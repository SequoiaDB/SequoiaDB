package com.sequoiadb.clustermanager;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-19222
 * @describe: 更新节点配置，options为null
 * @author wangkexin
 * @Date 2019.08.26
 * @version 1.00
 */
public class ClusterManager19222 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() {
        // test updateConfig(BSONObject configs)
        BSONObject configs = new BasicBSONObject();
        configs.put( "weight", 20 );
        DBCursor cursor = null;
        try {
            sdb.updateConfig( configs );
            BSONObject selector = new BasicBSONObject();
            selector.put( "weight", 1 );
            cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor.hasNext() ) {
                BSONObject actual = cursor.getNext();
                Assert.assertEquals( actual, configs );
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            sdb.deleteConfig( configs, new BasicBSONObject() );
        }

        // test updateConfig(BSONObject configs, BSONObject options)
        BSONObject configs2 = new BasicBSONObject();
        configs2.put( "weight", 50 );
        DBCursor cursor2 = null;
        try {
            sdb.updateConfig( configs2, null );
            BSONObject selector = new BasicBSONObject();
            selector.put( "weight", 1 );
            cursor2 = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor2.hasNext() ) {
                BSONObject actual = cursor2.getNext();
                Assert.assertEquals( actual, configs2 );
            }
        } finally {
            if ( cursor2 != null ) {
                cursor2.close();
            }
            sdb.deleteConfig( configs, new BasicBSONObject() );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
