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
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestCSSetAttributes15233 extends SdbTestBase {
    /**
     * description: create cs 1.cs.Attributes() modify
     * PageSize、LobPageSize、domian 2. connect cata db and check cs attribute
     * testcase: 15233 author: chensiqin date: 2018/04/27
     */

    private Sequoiadb sdb = null;
    private CollectionSpace localcs = null;
    private String localCsName = "cs15233";
    private String domainName = "domain15233";
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
    public void test15233() {
        localcs = sdb.createCollectionSpace( localCsName );
        String[] arr = new String[] { dataGroupNames.get( 0 ),
                dataGroupNames.get( 1 ) };
        BSONObject option = new BasicBSONObject();
        option.put( "Groups", arr );
        sdb.createDomain( domainName, option );
        option = new BasicBSONObject();
        option.put( "PageSize", 4096 );
        option.put( "LobPageSize", 4096 );
        option.put( "Domain", domainName );
        localcs.setAttributes( option );
        CheckCSAttribute( option );
        sdb.dropCollectionSpace( localCsName );
        sdb.dropDomain( domainName );
    }

    private void CheckCSAttribute( BSONObject expected ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        DBCursor cur = null;
        matcher.put( "Name", localCsName );
        ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
        Sequoiadb cataDB = cataRg.getMaster().connect();
        CollectionSpace sysCS = cataDB.getCollectionSpace( "SYSCAT" );
        DBCollection sysCL = sysCS.getCollection( "SYSCOLLECTIONSPACES" );
        DBQuery query = new DBQuery();
        query.setMatcher( matcher );
        cur = sysCL.query( query );
        Assert.assertNotNull( cur.getNext() );
        actual = cur.getCurrent();
        Assert.assertEquals( actual.get( "PageSize" ).toString(),
                expected.get( "PageSize" ).toString() );
        Assert.assertEquals( actual.get( "LobPageSize" ).toString(),
                expected.get( "LobPageSize" ).toString() );
        Assert.assertEquals( actual.get( "Domain" ).toString(),
                expected.get( "Domain" ).toString() );
        cur.close();
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
