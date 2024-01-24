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
import com.sequoiadb.testcommon.SdbTestBase;

public class TestAlterDomain15220 extends SdbTestBase {
    /**
     * description: alter domain attribute 1.create domain 2.domain.Alter()
     * domain.alter({Alter:[ {Name:"set attributes", Args:{AutoSplit:true}},
     * {Name:"set attributes", Args: {Name:‘test’}}, {Name:"add groups", Args:
     * {Groups:['group1']}}], Options:{IgnoreException:true}} ) testcase: 15220
     * author: chensiqin date: 2018/04/27
     */
    private Sequoiadb sdb = null;
    private Domain domain = null;
    private String domainName = "domain15220";
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
    public void test15220() {
        String[] arr = new String[] { dataGroupNames.get( 0 ),
                dataGroupNames.get( 1 ) };
        BSONObject option = new BasicBSONObject();
        option.put( "Groups", arr );
        domain = sdb.createDomain( domainName, option );

        BSONObject alterList = new BasicBSONList();
        BSONObject alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "Name", "names11" ) );
        alterList.put( Integer.toString( 0 ), alterBson );

        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "set attributes" );
        alterBson.put( "Args", new BasicBSONObject( "AutoSplit", true ) );
        alterList.put( Integer.toString( 1 ), alterBson );

        arr = new String[] { dataGroupNames.get( 2 ) };
        alterBson = new BasicBSONObject();
        alterBson.put( "Name", "add groups" );
        alterBson.put( "Args", new BasicBSONObject( "Groups", arr ) );
        alterList.put( Integer.toString( 2 ), alterBson );

        option = new BasicBSONObject();
        option.put( "Alter", alterList );
        option.put( "Options", new BasicBSONObject( "IgnoreException", true ) );
        domain.alterDomain( option );
        CheckDomainInfo();
        sdb.dropDomain( domainName );
    }

    public void CheckDomainInfo() {

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", domainName );
        DBCursor cur = sdb.listDomains( matcher, null, null, null );
        Assert.assertNotNull( cur.getNext() );
        BSONObject obj = cur.getCurrent();
        // check name group autosplit
        Assert.assertEquals( obj.get( "Name" ).toString(), domainName );
        Assert.assertEquals( ( boolean ) obj.get( "AutoSplit" ), true );
        BasicBSONList arr = ( BasicBSONList ) obj.get( "Groups" );

        for ( int i = 0; i < arr.size(); i++ ) {
            BSONObject element = ( BSONObject ) arr.get( i );
            dataGroupNames.contains( element.get( "GroupName" ).toString() );
            dataGroupNames.remove( element.get( "GroupName" ).toString() );
        }
        Assert.assertEquals( 0, dataGroupNames.size() );
        cur.close();
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
