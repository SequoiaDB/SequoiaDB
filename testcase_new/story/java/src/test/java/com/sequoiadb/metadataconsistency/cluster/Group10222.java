package com.sequoiadb.metadataconsistency.cluster;

import java.util.Random;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.metadataconsistency.data.MetaDataUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * TestLink: seqDB-10221: concurrency[remove group]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Group10222 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String rgName = "rg10222";
    private Random random = new Random();
    private int msec = 100;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode and group number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, "
                            + "skip the testCase." );
        }
        MetaDataUtils.clearGroup( sdb, rgName );
        sdb.createReplicaGroup( rgName );

    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            MetaDataUtils.clearGroup( sdb, rgName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {

        RemoveRG removeRG = new RemoveRG();
        removeRG.start();

        MetaDataUtils.sleep( random.nextInt( msec ) );
        removeRG.start();

        if ( !removeRG.isSuccess() ) {
            Assert.fail( removeRG.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkRGOfCatalog( rgName );
    }

    private class RemoveRG extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.removeReplicaGroup( rgName );
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

}