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
public class Alter14997B extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String domainName = "domain14997B";
    List< String > groupNames = new ArrayList< String >();
    private String[] expGroups = new String[ 1 ];

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
        sdb.createDomain( domainName, options );

        expGroups[ 0 ] = groupNames.get( 1 );
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
        Assert.assertEquals( actGroupName, expGroups[ 0 ],
                "check domain group name" );
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
                domain.setGroups( new BasicBSONObject( "Groups", expGroups ) );
            }
        }
    }

    public class SetAttributes extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Domain domain = db.getDomain( domainName );
                domain.setAttributes(
                        new BasicBSONObject( "Groups", expGroups ) );
            }
        }
    }
}
