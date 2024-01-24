package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestCS15150 extends SdbTestBase {
    /**
     * description:
     * createDomain1(group1,group2,group3),createDomain(group1,group2) cs
     * setDomain(domain1),setDomain(domain2),removeDomain(domain2),connect cata
     * node check result in every step testcase: 15150 author: chensiqin date:
     * 2018/04/28
     */

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl15150";
    private String localCsName = "cs15150";
    private String domainName1 = "domain15150_1";
    private String domainName2 = "domain15150_2";
    private List< String > dataGroupNames = new ArrayList< String >();

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( coordAddr, "", "" );
        CommLib commLib = new CommLib();
        if ( commLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
        dataGroupNames = commLib.getDataGroups( this.sdb );
        if ( dataGroupNames.size() < 3 ) {
            throw new SkipException( "data group less 3" );
        }
    }

    @Test
    public void test15150() {
        // create domain and cl
        createDomians();
        // setdomain1
        BSONObject option = new BasicBSONObject();
        option.put( "Domain", domainName1 );
        cs.setDomain( option );
        checkCSCataInfo( domainName1 );

        // setdomain2
        option = new BasicBSONObject();
        option.put( "Domain", domainName2 );
        cs.setDomain( option );
        checkCSCataInfo( domainName2 );

        // remove domain
        cs.removeDomain();
        checkCSCataInfo( "" );

        sdb.dropCollectionSpace( localCsName );
        sdb.dropDomain( domainName1 );
        sdb.dropDomain( domainName2 );
    }

    public void createDomians() {
        // create domain1
        BSONObject option = new BasicBSONObject();
        String[] arr = new String[] { dataGroupNames.get( 0 ),
                dataGroupNames.get( 1 ), dataGroupNames.get( 2 ) };
        option.put( "Groups", arr );
        Domain domain1 = sdb.createDomain( domainName1, option );

        // create domian2
        option = new BasicBSONObject();
        arr = new String[] { dataGroupNames.get( 0 ), dataGroupNames.get( 1 ) };
        option.put( "Groups", arr );
        Domain domain2 = sdb.createDomain( domainName2, option );

        // createcl
        cs = sdb.createCollectionSpace( localCsName );
        option = new BasicBSONObject();
        option.put( "Group", dataGroupNames.get( 0 ) );
        cs.createCollection( clName, option );

    }

    public void checkCSCataInfo( String expectDomain ) {
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", localCsName );
        ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
        Sequoiadb cataDB = cataRg.getMaster().connect();
        CollectionSpace sysCS = cataDB.getCollectionSpace( "SYSCAT" );
        DBCollection sysCL = sysCS.getCollection( "SYSCOLLECTIONSPACES" );
        query.setMatcher( matcher );
        DBCursor cur = sysCL.query( query );
        Assert.assertNotNull( cur.getNext() );
        BSONObject re = cur.getCurrent();
        if ( expectDomain != "" ) {
            Assert.assertEquals( re.get( "Domain" ).toString(), expectDomain );
        } else {
            Assert.assertEquals( re.get( "Domain" ), null );
        }
        cur.close();
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
