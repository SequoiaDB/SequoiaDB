package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestAlterCappedCL15227 extends SdbTestBase {
    /**
     * description: alter cappedcl attribute 1.cs.enableCapped({Capped:true})
     * 2.cs.createCL("test",{Capped:true,Size:10000,Max:1000,AutoIndexId:false})
     * 3.check cl attibute info 4.disableCapped() 5.check result 6.delete cs.cl
     * and cs.disableCapped() 7. check result testcase: 15227 author: chensiqin
     * date: 2018/04/27
     */
    private Sequoiadb sdb = null;
    private CollectionSpace localcs = null;
    private String localCsName = "cs15227";
    private String clName = "cl15227";

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( coordAddr, "", "" );
        CommLib commLib = new CommLib();
        if ( commLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
    }

    @Test
    public void test15227() {
        BSONObject clop = new BasicBSONObject();
        clop.put( "Capped", true );
        localcs = sdb.createCollectionSpace( localCsName, clop );
        clop = new BasicBSONObject();
        clop.put( "Size", 1024 );
        clop.put( "Max", 1000 );
        clop.put( "AutoIndexId", false );
        clop.put( "Capped", true );
        localcs.createCollection( clName, clop );
        checkCLAttribute( true, clop );

        try {
            localcs.disableCapped();
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -275 );
        }
        localcs.dropCollection( clName );
        localcs.disableCapped();
        checkCLAttribute( false, clop );

        sdb.dropCollectionSpace( localCsName );
    }

    private void checkCLAttribute( boolean capped, BSONObject expected ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        DBCursor cur = null;
        if ( capped ) {
            matcher.put( "Name", localCsName + "." + clName );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            long expectSize = 1024 * 1024 * 1024;
            Assert.assertEquals( actual.get( "Size" ).toString(),
                    expectSize + "" );
            Assert.assertEquals( actual.get( "Max" ).toString(),
                    expected.get( "Max" ).toString() );
            // Assert.assertEquals(actual.get("AutoIndexId").toString(),
            // expected.get("AutoIndexId").toString());
            Assert.assertEquals( actual.get( "AttributeDesc" ).toString(),
                    "NoIDIndex | Capped" );
        } else {
            matcher.put( "Name", localCsName );
            ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
            Sequoiadb cataDB = cataRg.getMaster().connect();
            CollectionSpace sysCS = cataDB.getCollectionSpace( "SYSCAT" );
            DBCollection sysCL = sysCS.getCollection( "SYSCOLLECTIONSPACES" );
            DBQuery query = new DBQuery();
            query.setMatcher( matcher );
            cur = sysCL.query( query );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            Assert.assertEquals( actual.get( "Type" ).toString(), 0 + "" );
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
