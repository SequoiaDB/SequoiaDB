package com.sequoiadb.lob.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7842: remove lobs*
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestRemoveLobs7842 extends SdbTestBase {
    @DataProvider(name = "lobSizeProvider")
    public Object[][] generateLobSize() {
        return new Object[][] {
                // the parameter : lobsize
                // test a ：it is just a piece with lobmeta，only on one group
                new Object[] { 1024 * 3 },
                // test b : it is many pieces,split to all groups
                new Object[] { 1024 * 1024 }, };
    }

    private String clName = "cl_lob7842";
    private String csName = "cs_lob7842";
    private String domainName = "domain_lob7842";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        createDomain( sdb );
        cs = sdb.createCollectionSpace( csName, ( BSONObject ) JSON
                .parse( "{LobPageSize:4096,Domain:'" + domainName + "'}" ) );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',ReplSize:0,AutoSplit:true}";
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );
    }

    @Test(dataProvider = "lobSizeProvider")
    public void removeLob( int lobSize ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            byte[] wlobBuff = LobOprUtils.getRandomBytes( lobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );
            dbcl.removeLob( oid );
            // check the remove result
            DBCursor listCursor1 = dbcl.listLobs();
            Assert.assertEquals( listCursor1.hasNext(), false,
                    "list lob not null" );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) )
                sdb.dropCollectionSpace( csName );
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } finally {
            if ( null != sdb )
                sdb.close();
        }
    }

    private void createDomain( Sequoiadb sdb ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }
        BSONObject options = new BasicBSONObject();
        options = ( BSONObject ) JSON.parse( "{'Groups': ["
                + LobOprUtils.chooseDataGroups( sdb ) + "],AutoSplit:true}" );
        sdb.createDomain( domainName, options );
    }
}
