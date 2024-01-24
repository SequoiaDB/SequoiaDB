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

/**
 * @FileName:TestDomain15147 1.create domain 2.addGroups and check domain
 *                           attribute 3.setGroups and check domain attribute
 *                           4.removeGroups and check domain attribute
 * @author chensiqin
 * @version 1.00
 *
 */
public class TestDomain15147 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Domain domain = null;
    private String domainName = "domain15147";
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
    public void test15147() {
        String[] arr = new String[] { dataGroupNames.get( 0 ) };
        BSONObject option = new BasicBSONObject();
        option.put( "Groups", arr );
        domain = sdb.createDomain( domainName, option );

        // 1.addGroups and check domain attribute
        arr = new String[] { dataGroupNames.get( 1 ), dataGroupNames.get( 2 ) };
        option = new BasicBSONObject();
        option.put( "Groups", arr );
        domain.addGroups( option );
        BasicBSONList expectedList = new BasicBSONList();
        expectedList.add( dataGroupNames.get( 0 ) );
        expectedList.add( dataGroupNames.get( 1 ) );
        expectedList.add( dataGroupNames.get( 2 ) );
        CheckDomainInfo( expectedList );

        // 2.setGroups and check domain attribute
        arr = new String[] { dataGroupNames.get( 0 ), dataGroupNames.get( 1 ) };
        option = new BasicBSONObject();
        option.put( "Groups", arr );
        expectedList = new BasicBSONList();
        expectedList.add( dataGroupNames.get( 0 ) );
        expectedList.add( dataGroupNames.get( 1 ) );
        domain.setGroups( option );
        CheckDomainInfo( expectedList );

        // 3.removeGroups and check domain attribute
        arr = new String[] { dataGroupNames.get( 0 ) };
        option = new BasicBSONObject();
        option.put( "Groups", arr );
        domain.removeGroups( option );
        expectedList = new BasicBSONList();
        expectedList.add( dataGroupNames.get( 1 ) );
        CheckDomainInfo( expectedList );

        sdb.dropDomain( domainName );
    }

    public void CheckDomainInfo( BasicBSONList expected ) {

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", domainName );
        DBCursor cur = sdb.listDomains( matcher, null, null, null );
        Assert.assertTrue( cur.hasNext() );
        BSONObject obj = cur.getNext();
        BasicBSONList groupInfo = ( BasicBSONList ) obj.get( "Groups" );
        for ( int i = 0; i < groupInfo.size(); i++ ) {
            BSONObject doc = ( BSONObject ) groupInfo.get( i );
            expected.contains( doc.get( "GroupName" ).toString() );
            expected.remove( doc.get( "GroupName" ).toString() );
        }
        Assert.assertEquals( 0, expected.size() );
        cur.close();
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
