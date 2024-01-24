package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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

public class TestCS15149 extends SdbTestBase {
    /**
     * description: cs.Alter() modify cs's Name、LobPageSize、PageSize attribute,
     * and chek result sunch as: a. alter（{PageSize:4096，LobPageSize：2048}） b.
     * db.newcs.alter({Alter:[ {Name:"set attributes", Args:{PageSize:4096}},
     * {Name:"set attributes", Args: {Name:“cs”}}, {Name:"set attributes", Args:
     * {PageSize:8192}}], Options:{IgnoreException:true}} ) testcase: 15149
     * author: chensiqin date: 2018/04/26
     */
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String localCsName = "cs15149";
    private String domainName = "domain15149";
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
    public void test15149() {
        cs = sdb.createCollectionSpace( localCsName );
        BSONObject alterList = new BasicBSONList();
        BSONObject alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "PageSize", 4096 ) );
        alterList.put( Integer.toString( 0 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "LobPageSize", 4096 ) );
        alterList.put( Integer.toString( 1 ), alterBson );

        BSONObject options = new BasicBSONObject();
        options.put( "Alter", alterList );
        cs.alterCollectionSpace( options );
        checkCSCataInfo( false );

        BSONObject option = new BasicBSONObject();

        String[] arr = new String[] { dataGroupNames.get( 0 ),
                dataGroupNames.get( 1 ) };
        option.put( "Groups", arr );
        sdb.createDomain( domainName, option );

        alterList = new BasicBSONList();
        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "PageSize", 8192 ) );
        alterList.put( Integer.toString( 0 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "LobPageSize", 8192 ) );
        alterList.put( Integer.toString( 1 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Name", "cs" ) );
        alterList.put( Integer.toString( 2 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Domain", domainName ) );
        alterList.put( Integer.toString( 3 ), alterBson );

        options = new BasicBSONObject();
        options.put( "Alter", alterList );

        alterBson = new BasicBSONObject();
        alterBson.put( "IgnoreException", true );
        options.put( "Options", alterBson );
        cs.alterCollectionSpace( options );
        checkCSCataInfo( true );

        sdb.dropCollectionSpace( localCsName );
        sdb.dropDomain( domainName );
    }

    public void checkCSCataInfo( boolean ignoreException ) {
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        matcher.put( "Name", localCsName );
        ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
        Sequoiadb cataDB = cataRg.getMaster().connect();
        CollectionSpace sysCS = cataDB.getCollectionSpace( "SYSCAT" );
        DBCollection sysCL = sysCS.getCollection( "SYSCOLLECTIONSPACES" );
        query.setMatcher( matcher );
        DBCursor cur = sysCL.query( query );
        if ( ignoreException ) {
            Assert.assertTrue( cur.hasNext() );
            actual = cur.getNext();
            Assert.assertEquals( actual.get( "PageSize" ).toString(),
                    8192 + "" );
            Assert.assertEquals( actual.get( "LobPageSize" ).toString(),
                    8192 + "" );
            Assert.assertEquals( actual.get( "Domain" ).toString(),
                    domainName );
        } else {
            Assert.assertTrue( cur.hasNext() );
            actual = cur.getNext();
            Assert.assertEquals( actual.get( "PageSize" ).toString(),
                    4096 + "" );
            Assert.assertEquals( actual.get( "LobPageSize" ).toString(),
                    4096 + "" );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
