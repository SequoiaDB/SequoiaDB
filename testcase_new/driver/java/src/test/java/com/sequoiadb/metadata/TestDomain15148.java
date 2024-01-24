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

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestDomain15148 extends SdbTestBase {
    /**
     * description: createDomain createdomain and alter domain and check
     * testcase: 15148 author: chensiqin date: 2018/04/28
     */
    private Sequoiadb sdb = null;
    private String domainName1 = "domain15148_1";
    private String domainName2 = "domain15148_2";
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
    public void test15148() {
        BSONObject option = new BasicBSONObject();
        String[] arr = new String[] { dataGroupNames.get( 0 ),
                dataGroupNames.get( 1 ) };
        option.put( "Groups", arr );
        Domain domain = sdb.createDomain( domainName1, option );
        // alter domain attribute
        option = new BasicBSONObject();
        arr = new String[] { dataGroupNames.get( 1 ), dataGroupNames.get( 2 ) };
        option.put( "Groups", arr );
        option.put( "Name", domainName2 );
        option.put( "AutoSplit", true );
        try {
            domain.setAttributes( option );
        } catch ( BaseException e ) {
            Assert.assertEquals( -6, e.getErrorCode() );
        }

        option = new BasicBSONObject();
        arr = new String[] { dataGroupNames.get( 1 ), dataGroupNames.get( 2 ) };
        option.put( "Groups", arr );
        option.put( "AutoSplit", true );
        domain.setAttributes( option );
        CheckDomainInfo( domainName1, dataGroupNames.get( 1 ),
                dataGroupNames.get( 2 ) );

        sdb.dropDomain( domainName1 );
    }

    public void CheckDomainInfo( String name, String groupnam1,
            String groupname2 ) {

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", name );
        DBCursor cur = sdb.listDomains( matcher, null, null, null );
        Assert.assertTrue( cur.hasNext() );
        BSONObject obj = cur.getNext();
        // check name group autosplit
        Assert.assertEquals( name, obj.get( "Name" ).toString() );
        Assert.assertEquals( true, ( boolean ) obj.get( "AutoSplit" ) );
        BasicBSONList groupInfo = ( BasicBSONList ) obj.get( "Groups" );
        for ( int i = 0; i < groupInfo.size(); i++ ) {
            BSONObject doc = ( BSONObject ) groupInfo.get( i );
            if ( !groupnam1.equals( doc.get( "GroupName" ).toString() )
                    && !groupname2
                            .equals( doc.get( "GroupName" ).toString() ) ) {
                Assert.fail( "alter domain Groups result is error!" );
            }
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
