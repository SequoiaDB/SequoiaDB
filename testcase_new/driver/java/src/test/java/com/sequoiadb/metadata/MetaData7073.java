package com.sequoiadb.metadata;

import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.Assert;
import org.testng.SkipException;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import org.testng.annotations.BeforeClass;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7073.java TestLink: seqDB-7073/seqDB-7079
 * 
 * @author zhaoyu
 * @Date 2016.9.19
 * @version 1.00
 */

public class MetaData7073 extends SdbTestBase {

    private Sequoiadb sdb;
    private BSONObject domainOption = new BasicBSONObject();
    private ArrayList< String > replicaGroups = new ArrayList< String >();
    private BasicBSONObject domainMatcher = new BasicBSONObject();
    private BasicBSONObject domainSelect = new BasicBSONObject();
    private BasicBSONObject domainOrderby = new BasicBSONObject();

    private String domainName = "domain7073";

    private CommLib commlib = new CommLib();
    private String coordAddr;

    // connect sdb before test
    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
            commlib.dropDomainForClearEnv( sdb, domainName );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed " + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commlib.dropDomainForClearEnv( sdb, domainName );
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        // seqDB-7073
        // get replica group name
        replicaGroups = commlib.getDataGroups( sdb );

        // init domain arg
        domainOption.put( "Groups", replicaGroups );
        domainOption.put( "AutoSplit", true );
        boolean expectAutoSplit = true;

        // create domain
        commlib.createDomain( sdb, domainName, domainOption );

        // check result
        domainMatcher.put( "Name", domainName );
        domainSelect.put( "AutoSplit", 1 );
        domainSelect.put( "Groups", 1 );
        domainSelect.put( "Name", 1 );
        domainOrderby.put( "Name", 1 );
        commlib.checkDomainInfo( sdb, domainMatcher, domainSelect,
                domainOrderby, null, replicaGroups, domainName,
                expectAutoSplit );

        // get expect group name in domain
        domainOption.put( "AutoSplit", false );
        boolean expectAutoSplit1 = false;

        // alter domain
        commlib.alterDomain( sdb, domainName, domainOption );

        // get actual group name in domain
        commlib.checkDomainInfo( sdb, domainMatcher, domainSelect,
                domainOrderby, null, replicaGroups, domainName,
                expectAutoSplit1 );

        // drop domain
        commlib.dropDomain( sdb, domainName );

        // check result
        Assert.assertFalse( sdb.isDomainExist( domainName ) );

        // seqDB-7079
        String csName = "cs7079";
        BSONObject csOption = new BasicBSONObject();
        csOption.put( "Domain", domainName );
        try {
            sdb.createCollectionSpace( csName, csOption );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -214 ) {
                Assert.assertTrue( false,
                        "create cs, errMsg:" + e.getMessage() );
            }
        }
    }
}
