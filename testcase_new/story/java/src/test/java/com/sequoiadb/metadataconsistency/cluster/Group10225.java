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
 * TestLink: seqDB-10225、seqDB-10226: concurrency[createRG, removeRG]
 * 
 * @author xiaoni huang init
 * @Date 2016.10.24
 */

public class Group10225 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String rgName = "rg10225";
    private Random random = new Random();
    private int number = 3;
    private int msec = 1000;

    @BeforeClass
    public void setUp() {
        // start time
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // judge the mode and group number
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "The mode is standlone, or only one group, skip the testCase." );
        }
        MetaDataUtils.clearGroup( sdb, rgName );
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        try {
            MetaDataUtils.clearGroup( sdb, rgName );
        } finally {
            sdb.close();
        }
    }

    @Test(invocationCount = 1, threadPoolSize = 1)
    public void test() {

        CreateRG createRG = new CreateRG();
        createRG.start();

        RemoveRG removeRG = new RemoveRG();
        MetaDataUtils.sleep( random.nextInt( msec ) );
        removeRG.start();

        if ( !( createRG.isSuccess() && removeRG.isSuccess() ) ) {
            Assert.fail( createRG.getErrorMsg() + removeRG.getErrorMsg() );
        }

        // check results
        MetaDataUtils.checkRGOfCatalog( rgName );
    }

    private class CreateRG extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.createReplicaGroup(
                        rgName + "_" + random.nextInt( number ) );
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode != -153 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private class RemoveRG extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                String rgName2 = rgName + "_" + random.nextInt( number );
                db.removeReplicaGroup( rgName2 );
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