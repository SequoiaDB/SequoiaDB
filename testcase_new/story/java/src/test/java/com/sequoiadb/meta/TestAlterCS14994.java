package com.sequoiadb.meta;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: TestAlterCS14994.java test content:Concurrent alter cs ,alter the
 * same field testlink case:seqDB-14994
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestAlterCS14994 extends SdbTestBase {
    private String csName = "altercs14994";
    private static Sequoiadb sdb = null;
    private String domainName1 = "domain14994a";
    private String domainName2 = "domain14994b";
    private List< String > dataGroupNames = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
        dataGroupNames = CommLib.getDataGroupNames( sdb );
        if ( dataGroupNames.size() < 3 ) {
            throw new SkipException( "data group less 3" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isDomainExist( domainName1 ) ) {
            sdb.dropDomain( domainName1 );
        }
        if ( sdb.isDomainExist( domainName2 ) ) {
            sdb.dropDomain( domainName2 );
        }

        sdb.createCollectionSpace( csName );

        BSONObject options1 = ( BSONObject ) JSON
                .parse( "{Groups:[ '" + dataGroupNames.get( 1 ) + "','"
                        + dataGroupNames.get( 0 ) + "']}" );
        BSONObject options2 = ( BSONObject ) JSON
                .parse( "{Groups:[ '" + dataGroupNames.get( 1 ) + "','"
                        + dataGroupNames.get( 2 ) + "']}" );
        sdb.createDomain( domainName1, options1 );
        sdb.createDomain( domainName2, options2 );
    }

    @Test
    public void testAlterCl() {
        AlterUseEnable alterTask1 = new AlterUseEnable();
        AlterUseSet alterTask2 = new AlterUseSet();
        AlterSameValue alterTask3 = new AlterSameValue();
        AlterDiffValue alterTask4 = new AlterDiffValue();
        alterTask1.start();
        alterTask2.start();
        alterTask3.start( 100 );
        alterTask4.start();

        Assert.assertTrue( alterTask1.isSuccess(), alterTask1.getErrorMsg() );
        Assert.assertTrue( alterTask2.isSuccess(), alterTask2.getErrorMsg() );
        Assert.assertTrue( alterTask3.isSuccess(), alterTask3.getErrorMsg() );
        Assert.assertTrue( alterTask4.isSuccess(), alterTask4.getErrorMsg() );
        checkAlterResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            sdb.dropDomain( domainName1 );
            sdb.dropDomain( domainName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AlterUseEnable extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{Domain:'" + domainName1 + "'}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private class AlterUseSet extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{Domain:'" + domainName2 + "'}" );
                dbcs.setDomain( options );
            }
        }
    }

    private class AlterSameValue extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{PageSize:16384}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private class AlterDiffValue extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{LobPageSize:16384}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private void checkAlterResult() {
        Sequoiadb cataDB = null;
        try {
            ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
            cataDB = cataRg.getMaster().connect();
            DBCollection dbcl = cataDB.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONSPACES" );
            BSONObject matcher = new BasicBSONObject();
            DBQuery query = new DBQuery();
            matcher.put( "Name", csName );
            query.setMatcher( matcher );
            DBCursor cur = dbcl.query( query );
            int actLobPageSize = 0;
            int actPageSize = 0;
            int expLobPageSize = 16384;
            int expPageSize = 16384;
            String actDomain = null;
            while ( cur.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cur.getNext();
                actLobPageSize = ( int ) doc.get( "LobPageSize" );
                actPageSize = ( int ) doc.get( "PageSize" );
                actDomain = ( String ) doc.get( "Domain" );
            }
            cur.close();
            Assert.assertEquals( actLobPageSize, expLobPageSize );
            Assert.assertEquals( actPageSize, expPageSize );

            String[] expDomains = { domainName1, domainName2 };
            List< String > expDomainsList = Arrays.asList( expDomains );
            Assert.assertTrue( expDomainsList.contains( actDomain ),
                    "domain for one of the modified values!" );
        } finally {
            cataDB.close();
        }
    }
}
