package com.sequoiadb.metadataconsistency.cluster;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10223: concurrency[removeRG, alterDomain]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Group10223 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private static ArrayList< String > dataGroups = null;
    private String rgName = "rg10223";
    private String domainName = "dm10223";
    private Random random = new Random();
    private int msec = 100;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode or group number or node number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb )
                || MetaDataUtils.oneCataNode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone or only one group or one node, "
                            + "skip the testCase." );
        }
        MetaDataUtils.clearDomain( sdb, domainName );
        MetaDataUtils.clearGroup( sdb, rgName );

        dataGroups = MetaDataUtils.getDataGroupNames( sdb );

        ReplicaGroup rg = sdb.createReplicaGroup( rgName );
        createNode();
        rg.start();

        createDomain( sdb );
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            MetaDataUtils.clearDomain( sdb, domainName );
            MetaDataUtils.clearGroup( sdb, rgName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {

        AlterDomain alterDomain = new AlterDomain();
        alterDomain.start();

        RemoveRG removeRG = new RemoveRG();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        removeRG.start();

        if ( !( removeRG.isSuccess() && alterDomain.isSuccess() ) ) {
            Assert.fail( removeRG.getErrorMsg() + alterDomain.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkRGOfCatalog( rgName );
        MetaDataUtils.checkDomainOfCatalog( domainName );
    }

    private class RemoveRG extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.removeReplicaGroup( rgName );
            } finally {
                db.close();
            }
        }
    }

    private class AlterDomain extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

                String[] groups = { dataGroups.get( 0 ) };
                BSONObject opt = new BasicBSONObject();
                opt.put( "Groups", groups );
                sdb.getDomain( domainName ).alterDomain( opt );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -154 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    public void createNode() {
        for ( int i = 0; i < 3; i++ ) {
            MetaDataUtils.createNode( sdb, rgName,
                    SdbTestBase.reservedPortBegin, SdbTestBase.reservedPortEnd,
                    SdbTestBase.reservedDir );
        }
    }

    public void createDomain( Sequoiadb sdb ) {
        String[] rgArr = { rgName };
        BSONObject opt = new BasicBSONObject();
        opt.put( "Groups", rgArr );
        sdb.createDomain( domainName, opt );
    }

}