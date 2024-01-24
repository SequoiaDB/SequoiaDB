package com.sequoiadb.meta;

import java.util.ArrayList;
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
 * FileName: TestAlterCS14995.java test content:Concurrent alter cs different
 * attribute,eg: pageSize/domain/LobPageSize testlink case:seqDB-14995
 * 
 * @author wuyan
 * @Date 2018.4.22
 * @version 1.00
 */
public class TestAlterCS14995 extends SdbTestBase {
    private String csName = "altercs14995";
    private static Sequoiadb sdb = null;
    private String domainName1 = "domain14995a";
    private String domainName2 = "domain14995b";
    private int lobPageSize = 4096;
    private int pageSize = 65536;
    private int curLobPageSize = 32768;
    private int curPageSize = 4096;
    private List< String > dataGroupNames = new ArrayList< String >();

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

        BSONObject options1 = ( BSONObject ) JSON
                .parse( "{Groups:[ '" + dataGroupNames.get( 1 ) + "','"
                        + dataGroupNames.get( 0 ) + "']}" );
        BSONObject options2 = ( BSONObject ) JSON
                .parse( "{Groups:[ '" + dataGroupNames.get( 1 ) + "','"
                        + dataGroupNames.get( 2 ) + "']}" );
        sdb.createDomain( domainName1, options1 );
        sdb.createDomain( domainName2, options2 );

        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", pageSize );
        options.put( "Domain", domainName1 );
        options.put( "LobPageSize", lobPageSize );
        sdb.createCollectionSpace( csName, options );
    }

    @Test
    public void testAlterCl() {
        AlterDomain alterTask1 = new AlterDomain();
        AlterPageSize alterTask2 = new AlterPageSize();
        AlterLobPageSize alterTask3 = new AlterLobPageSize();
        alterTask1.start();
        alterTask2.start();
        alterTask3.start();

        Assert.assertTrue( alterTask1.isSuccess(), alterTask1.getErrorMsg() );
        Assert.assertTrue( alterTask2.isSuccess(), alterTask2.getErrorMsg() );
        Assert.assertTrue( alterTask3.isSuccess(), alterTask3.getErrorMsg() );
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

    private class AlterDomain extends SdbThreadBase {
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{Domain:'" + domainName2 + "'}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private class AlterPageSize extends SdbThreadBase {
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{PageSize:" + curPageSize + "}" );
                dbcs.setAttributes( options );
            }
        }
    }

    private class AlterLobPageSize extends SdbThreadBase {
        public void exec() throws BaseException {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace dbcs = sdb.getCollectionSpace( csName );
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{LobPageSize:" + curLobPageSize + "}" );
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
            ;
            int actPageSize = 0;
            String actDomain = null;
            while ( cur.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cur.getNext();
                actLobPageSize = ( int ) doc.get( "LobPageSize" );
                actPageSize = ( int ) doc.get( "PageSize" );
                actDomain = ( String ) doc.get( "Domain" );
            }
            cur.close();
            Assert.assertEquals( actLobPageSize, curLobPageSize );
            Assert.assertEquals( actPageSize, curPageSize );
            Assert.assertEquals( actDomain, domainName2 );
        } finally {
            cataDB.close();
        }
    }
}
