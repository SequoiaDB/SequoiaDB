package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.Collections;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName:
 * MetaData7074.java TestLink: seqDB-7074/seqDB-7077
 * 
 * @author zhaoyu
 * @Date 2016.9.19
 * @version 1.00
 */
public class MetaData7074 extends SdbTestBase {
    private Sequoiadb sdb;
    private BSONObject domainOption = new BasicBSONObject();
    private ArrayList< String > replicaGroups = new ArrayList< String >();

    private String domainName = "mydomain70741";
    private String csName1 = "cs70741";
    private String csName2 = "cs70742";

    private CommLib commlib = new CommLib();
    private String coordAddr;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException(
                        "run mode is standalone,test case skip" );
            }
            if ( true == sdb.isCollectionSpaceExist( csName1 ) ) {
                sdb.dropCollectionSpace( csName1 );
            }
            if ( true == sdb.isCollectionSpaceExist( csName2 ) ) {
                sdb.dropCollectionSpace( csName2 );
            }
            commlib.dropDomainForClearEnv( sdb, domainName );
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
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

    @DataProvider(name = "createNameProvider")
    public Object[][] createName() {
        return new Object[][] { { "mydomain70741", "cs70741", "cl70741" },
                { "mydomain70741", "cs70741", "cl70742" },
                { "mydomain70741", "cs70742", "cl70741" },
                { "mydomain70741", "cs70742", "cl70742" }, };
    }

    // prepare env for list interface
    @Test(dataProvider = "createNameProvider", priority = 0)
    public void prepareEnv( String domainName, String csName, String clName ) {
        try {
            // get replica group name
            replicaGroups = commlib.getDataGroups( sdb );

            // init domain option
            domainOption.put( "Groups", replicaGroups );
            domainOption.put( "AutoSplit", true );

            // create domain
            if ( false == sdb.isDomainExist( domainName ) ) {
                sdb.createDomain( domainName, domainOption );
            }

            // create cs
            BSONObject csOption = new BasicBSONObject();
            csOption.put( "Domain", domainName );
            if ( false == sdb.isCollectionSpaceExist( csName ) ) {
                sdb.createCollectionSpace( csName, csOption );
            }

            // create cl
            if ( false == sdb.getCollectionSpace( csName )
                    .isCollectionExist( clName ) ) {
                sdb.getCollectionSpace( csName ).createCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "create domain/cs/cl failed, errMsg:" + e.getMessage() );
        }
    }

    @SuppressWarnings("unchecked")
    @Test(priority = 1)
    public void test() {
        // get domain groups name
        boolean expectAutoSplit = ( boolean ) domainOption.get( "AutoSplit" );
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", domainName );
        commlib.checkDomainInfo( sdb, matcher, null, null, null, replicaGroups,
                domainName, expectAutoSplit );

        BasicBSONList csNameArr = getCSInDomain( domainName );
        List< String > expectCSNamelist = new LinkedList< String >();
        expectCSNamelist.add( csName1 );
        expectCSNamelist.add( csName2 );

        List actList = csNameArr;
        Collections.sort( actList );
        Collections.sort( expectCSNamelist );
        Assert.assertEquals( actList, expectCSNamelist, "cs name actual:"
                + csNameArr + ";the expect :" + expectCSNamelist.toString() );

        BasicBSONList clNameArr = getCLInDomain( domainName );
        List< String > expectCLNameArr = new LinkedList< String >();
        String clName1 = "cl70741";
        String clName2 = "cl70742";
        expectCLNameArr.add( csName1 + "." + clName1 );
        expectCLNameArr.add( csName1 + "." + clName2 );
        expectCLNameArr.add( csName2 + "." + clName1 );
        expectCLNameArr.add( csName2 + "." + clName2 );
        List clNameList = clNameArr;
        Collections.sort( clNameList );
        Collections.sort( expectCLNameArr );
        Assert.assertEquals( clNameList, expectCLNameArr,
                "cl name actual:" + clNameList.toString() + "the expect :"
                        + expectCLNameArr.toString() );

        Sequoiadb domainSdb = sdb.getDomain( domainName ).getSequoiadb();
        Assert.assertEquals( domainSdb, sdb,
                "sdb actual:" + domainSdb + ";the expect :" + sdb );

        // get domain, check result
        String domainNameGet = sdb.getDomain( domainName ).getName();
        Assert.assertEquals( domainNameGet, domainName, "domain name actual:"
                + domainNameGet + ";the expect :" + domainName );
    }

    public void listCLInDomain( Domain domain ) {
        try {
            domain.listCLInDomain();
        } catch ( BaseException e ) {
            Assert.fail( "list cl in domain failed, errMsg:" + e.getMessage() );
        }
    }

    public void listCSInDomain( Domain domain ) {
        try {
            domain.listCSInDomain();
        } catch ( BaseException e ) {
            Assert.fail( "list cs in domain failed, errMsg:" + e.getMessage() );
        }
    }

    public BasicBSONList getCSInDomain( String domainName ) {
        DBCursor csInDomain = sdb.getDomain( domainName ).listCSInDomain();
        BasicBSONList csNameArr = new BasicBSONList();

        while ( csInDomain.hasNext() ) {
            BSONObject csObject = ( BSONObject ) csInDomain.getNext();
            String csName = ( String ) csObject.get( "Name" );
            csNameArr.add( csName );
        }
        csInDomain.close();
        return csNameArr;
    }

    public BasicBSONList getCLInDomain( String domainName ) {
        DBCursor clInDomain = sdb.getDomain( domainName ).listCLInDomain();
        BasicBSONList clNameArr = new BasicBSONList();
        while ( clInDomain.hasNext() ) {
            BSONObject clObject = ( BSONObject ) clInDomain.getNext();
            String clName = ( String ) clObject.get( "Name" );
            clNameArr.add( clName );
        }
        clInDomain.close();
        return clNameArr;

    }

}
