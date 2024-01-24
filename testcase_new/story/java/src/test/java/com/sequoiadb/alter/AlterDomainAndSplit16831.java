package com.sequoiadb.alter;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 *  @FileName: AlterDomainAndSplit16831
 *  @content alter domain和split并发
 *  @author chensiqin
 *  @Date 2018-12-21
 *  @version 1.00
 */

/**
 * update
 * 
 * @author wangkexin
 * @Date 2019-2-28
 */
public class AlterDomainAndSplit16831 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String domainName = "domain16831";
    private String localCSName = "cs^cs16831";
    private String clName = "cl16831";
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private List< String > rgList = new ArrayList< String >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "current environment less than two groups " );
        }
        if ( sdb.isCollectionSpaceExist( localCSName ) ) {
            sdb.dropCollectionSpace( localCSName );
        }
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }
    }

    @Test
    public void test16831() {
        rgList = CommLib.getDataGroupNames( sdb );
        prepareCLAndDomain();

        AlterDomainThread domainThread = new AlterDomainThread();
        SplitThread splitThread = new SplitThread();
        domainThread.start();
        splitThread.start();
        if ( domainThread.isSuccess() && !splitThread.isSuccess() ) {
            checkDomain( true );
            Assert.assertEquals( cl.getCount(), 1000 );
            ReplicaGroup rg = sdb.getReplicaGroup( rgList.get( 0 ) );
            Node srcGroupMaster = rg.getMaster();
            Sequoiadb localDB = srcGroupMaster.connect();
            CollectionSpace localCS = localDB.getCollectionSpace( localCSName );
            DBCollection localCL = localCS.getCollection( clName );
            Assert.assertEquals( localCL.getCount(), 1000 );
            // 检查错误码
            BaseException e = ( BaseException ) splitThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -216 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !domainThread.isSuccess() && splitThread.isSuccess() ) {
            Assert.assertEquals( cl.getCount(), 1000 );
            // 源组
            ReplicaGroup rg = sdb.getReplicaGroup( rgList.get( 0 ) );
            Node srcGroupMaster = rg.getMaster();
            Sequoiadb localDB = srcGroupMaster.connect();
            CollectionSpace localCS = localDB.getCollectionSpace( localCSName );
            DBCollection srcCL = localCS.getCollection( clName );
            // 目标组
            rg = sdb.getReplicaGroup( rgList.get( 1 ) );
            Node destGroupMaster = rg.getMaster();
            localDB = destGroupMaster.connect();
            localCS = localDB.getCollectionSpace( localCSName );
            DBCollection destCL = localCS.getCollection( clName );
            // 源组和目标组之和正确性
            Assert.assertEquals( srcCL.getCount() + destCL.getCount(), 1000 );
            BaseException e = ( BaseException ) domainThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -256 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
            checkDomain( false );
        } else if ( domainThread.isSuccess() && splitThread.isSuccess() ) {
            checkDomain( true );
            Assert.assertEquals( cl.getCount(), 1000 );
        } else if ( !domainThread.isSuccess() && !splitThread.isSuccess() ) {
            BaseException e1 = ( BaseException ) domainThread.getExceptions()
                    .get( 0 );
            BaseException e2 = ( BaseException ) splitThread.getExceptions()
                    .get( 0 );
            Assert.fail(
                    "domainThread and splitThread all failed \n domainThread : "
                            + e1.getMessage() + "\n splitThread : "
                            + e2.getMessage() );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( localCSName ) ) {
                sdb.dropCollectionSpace( localCSName );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    public void checkDomain( boolean isSuccess ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", domainName );
        BSONObject selector = new BasicBSONObject();
        selector.put( "Groups", 1 );
        DBCursor cur = sdb.listDomains( matcher, selector, null, null );
        BSONObject act = new BasicBSONObject();
        while ( cur.hasNext() ) {
            act = cur.getNext();
        }
        if ( isSuccess ) {
            Assert.assertTrue( act.toString().contains( rgList.get( 0 ) ) );
        } else {
            Assert.assertTrue( act.toString().contains( rgList.get( 0 ) ) );
            Assert.assertTrue( act.toString().contains( rgList.get( 1 ) ) );
        }
        cur.close();
    }

    public void prepareCLAndDomain() {
        BSONObject options = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        groups.put( "0", rgList.get( 0 ) );
        groups.put( "1", rgList.get( 1 ) );
        options.put( "Groups", groups );
        sdb.createDomain( domainName, options );

        options = new BasicBSONObject();
        options.put( "Domain", domainName );
        cs = sdb.createCollectionSpace( localCSName, options );

        options = new BasicBSONObject();
        options.put( "Group", rgList.get( 0 ) );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        cl = cs.createCollection( clName, options );

        for ( int i = 1; i <= 1000; i++ ) {
            cl.insert( new BasicBSONObject( "a", i ) );
        }

    }

    private class AlterDomainThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                Domain localdomain = db.getDomain( domainName );
                BSONObject options = new BasicBSONObject();
                BSONObject groups = new BasicBSONList();
                groups.put( "0", rgList.get( 0 ) );
                options.put( "Groups", groups );
                localdomain.alterDomain( options );
            } finally {
                db.close();
            }

        }
    }

    private class SplitThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( localCSName );
                DBCollection localcl = localcs.getCollection( clName );
                localcl.split( rgList.get( 0 ), rgList.get( 1 ), 50 );
            } finally {
                db.close();
            }

        }
    }
}
