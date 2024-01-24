package com.sequoiadb.lob.basicoperation;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7845: set AutoSplit of cs ,then write lobs
 * @author wuyan
 * @Date 2016.9.12
 * @update [2017.12.20]
 * @version 1.00
 */
public class TestLobAutoSplit7845 extends SdbTestBase {
    private String domainName = "domain7845";
    private String csName = "cs_lob7845";
    private String clName = "cl_lob7845";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        createDomain( sdb );
        cs = sdb.createCollectionSpace( csName, ( BSONObject ) JSON
                .parse( "{LobPageSize:4096,Domain:'" + domainName + "'}" ) );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',ReplSize:0,AutoSplit:true}";
        cs.createCollection( clName, ( BSONObject ) JSON.parse( clOptions ) );
    }

    // eg:with 30 threads to write 100 lob
    @Test(invocationCount = 100, threadPoolSize = 30)
    public void testAutoSplitLob() {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            // write lob
            int lobsize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = LobOprUtils.getRandomBytes( lobsize );
            ObjectId oid = LobOprUtils.createAndWriteLob( dbcl, wlobBuff );

            // read lob and check the lob data
            try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                        "lob data is wrong!the oid: " + oid.toString() );
            }
        }
    }

    @Test(dependsOnMethods = "testAutoSplitLob")
    public void checkSplitResult() {
        ArrayList< String > groupList = LobOprUtils.getDataGroups( sdb );
        double expErrorValue = 0.95;
        LobOprUtils.checkSplitResult( sdb, csName, clName, groupList,
                expErrorValue );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
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
