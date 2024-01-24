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
 * @FileName Alter14997.java
 * @Author luweikang
 * @Date 2019年3月11日
 */
public class Alter14997A extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String domainName = "domain14997A";
    List< String > groupNames = new ArrayList< String >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        groupNames.addAll( CommLib.getDataGroupNames( sdb ) );
        if ( groupNames.size() < 3 ) {
            throw new SkipException( "the group less than three" );
        }
        BSONObject options = new BasicBSONObject();
        String[] domain1RG = new String[ 1 ];
        domain1RG[ 0 ] = groupNames.get( 0 );
        options.put( "Groups", domain1RG );
        sdb.createDomain( domainName, options );
    }

    @SuppressWarnings("unchecked")
    @Test
    public void test() {
        SetGroups setGroups = new SetGroups();
        SetAttributes setAttributes = new SetAttributes();

        setGroups.start();
        setAttributes.start();

        Assert.assertTrue( setGroups.isSuccess(), setGroups.getErrorMsg() );
        Assert.assertTrue( setAttributes.isSuccess(),
                setAttributes.getErrorMsg() );

        DBCursor cur = sdb.listDomains(
                new BasicBSONObject( "Name", domainName ), null, null, null );
        BSONObject domainInfo = cur.getNext();
        List< BSONObject > group = ( List< BSONObject > ) domainInfo
                .get( "Groups" );
        String actGroupName = ( String ) group.get( 0 ).get( "GroupName" );
        Assert.assertTrue(
                actGroupName.equals( groupNames.get( 1 ) )
                        || actGroupName.equals( groupNames.get( 2 ) ),
                "check domain group name" );

        cur.close();
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
                String[] domainRG = new String[ 1 ];
                domainRG[ 0 ] = groupNames.get( 2 );
                options.put( "Groups", domainRG );
                domain.setAttributes( options );
            }
        }
    }
}
