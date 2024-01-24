package com.sequoiadb.clustermanager;

import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestLink: seqDB-15245
 * @describe: updateConfig(BSONObject configs, BSONObject options)
 *            deleteConfig(BSONObject configs, BSONObject options)
 * @author chensiqin
 * @Date 2018.03.28
 * @version 1.00
 */
public class ClusterManager15245 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private CommLib commlib = new CommLib();

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        sdb = new Sequoiadb( coordAddr, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }

    }

    @Test
    public void test15245() {
        List< String > dataGroupNames = commlib.getDataGroupNames( sdb );
        List< String > nodeList = commlib.getNodeAddress( sdb,
                dataGroupNames.get( 0 ) );
        String nodeAddress = nodeList.get( 0 );
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "weight", 20 );
        options.put( "GroupName", dataGroupNames.get( 0 ) );
        options.put( "HostName", nodeAddress.split( ":" )[ 0 ] );
        options.put( "svcname", nodeAddress.split( ":" )[ 1 ] );
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "GroupName", dataGroupNames.get( 0 ) );
        matcher.put( "SvcName", nodeAddress.split( ":" )[ 1 ] );
        matcher.put( "HostName", nodeAddress.split( ":" )[ 0 ] );
        BSONObject selector = new BasicBSONObject();
        selector.put( "weight", 1 );
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, matcher,
                selector, null );
        BSONObject defValue = new BasicBSONObject();
        while ( cursor.hasNext() ) {
            defValue = cursor.getNext();
        }
        cursor.close();

        sdb.updateConfig( configs, options );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, matcher, selector,
                null );
        BSONObject expected = new BasicBSONObject();
        expected.put( "weight", 20 );
        while ( cursor.hasNext() ) {
            BSONObject actual = cursor.getNext();
            Assert.assertEquals( actual, expected );
        }
        cursor.close();

        // deleteconfig
        sdb.deleteConfig( configs, options );
        cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, matcher, selector,
                null );
        while ( cursor.hasNext() ) {
            BSONObject actual = cursor.getNext();
            Assert.assertEquals( actual, defValue );
        }
        cursor.close();
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.close();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }
}
