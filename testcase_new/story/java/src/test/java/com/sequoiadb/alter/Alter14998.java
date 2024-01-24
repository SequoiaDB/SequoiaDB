package com.sequoiadb.alter;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName Alter14998.java
 * @Author luweikang
 * @Date 2019年3月11日
 */
public class Alter14998 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String domainName = "domain14998";
    List< String > groupNames = new ArrayList< String >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        groupNames.addAll( CommLib.getDataGroupNames( sdb ) );
        if ( groupNames.size() < 2 ) {
            throw new SkipException( "the group less than two" );
        }
        BSONObject options = new BasicBSONObject();
        String[] domain1RG = new String[ 1 ];
        domain1RG[ 0 ] = groupNames.get( 0 );
        options.put( "Groups", domain1RG );
        options.put( "AutoSplit", false );
        options.put( "AutoRebalance", false );
        sdb.createDomain( domainName, options );

    }

    @SuppressWarnings("unchecked")
    @Test
    public void test() {
        SetGroups setGroups = new SetGroups();
        AlterDomain alterDomain = new AlterDomain();
        SetAttributes setAttributes = new SetAttributes();

        setGroups.start();
        alterDomain.start();
        setAttributes.start();

        Assert.assertTrue( setGroups.isSuccess(), setGroups.getErrorMsg() );
        Assert.assertTrue( alterDomain.isSuccess(), alterDomain.getErrorMsg() );
        Assert.assertTrue( setAttributes.isSuccess(),
                setAttributes.getErrorMsg() );

        DBCursor cur = sdb.listDomains(
                new BasicBSONObject( "Name", domainName ), null, null, null );
        BSONObject domainInfo = cur.getNext();
        List< BSONObject > group = ( List< BSONObject > ) domainInfo
                .get( "Groups" );
        String actGroupName = ( String ) group.get( 0 ).get( "GroupName" );
        boolean actAutoSplit = ( boolean ) domainInfo.get( "AutoSplit" );
        boolean actAutoRebalance = ( boolean ) domainInfo
                .get( "AutoRebalance" );
        Assert.assertTrue( actGroupName.equals( groupNames.get( 1 ) ),
                "check domain group name" );
        Assert.assertEquals( actAutoSplit, true, "check domain AutoSplit" );
        Assert.assertEquals( actAutoRebalance, true,
                "check domain AutoRebalance" );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropDomain( domainName );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    public class SetGroups extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Domain domain = db.getDomain( domainName );
                BSONObject options = new BasicBSONObject();
                String[] domainRG = new String[ 1 ];
                domainRG[ 0 ] = groupNames.get( 1 );
                options.put( "Groups", domainRG );
                domain.setGroups( options );
            }
        }
    }

    public class SetAttributes extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Domain domain = db.getDomain( domainName );
                BSONObject options = new BasicBSONObject();
                options.put( "AutoRebalance", true );
                domain.setAttributes( options );
            }
        }
    }

    public class AlterDomain extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Domain domain = db.getDomain( domainName );
                BSONObject options = new BasicBSONObject();
                options.put( "AutoSplit", true );
                domain.alterDomain( options );
            }
        }
    }
}
