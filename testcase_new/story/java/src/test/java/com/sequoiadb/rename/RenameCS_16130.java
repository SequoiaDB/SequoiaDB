package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16130.java 修改cs名和removeDomain并发
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16130 extends SdbTestBase {

    private String csName = "renameCS_16130";
    private String newCSName = "renameCS_16130_new";
    private String clName = "renameCS_CL_16130";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String domainName = "domain16130";
    private int recordNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > rgNames = CommLib.getDataGroupNames( sdb );
        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", rgNames ) );
        cs = sdb.createCollectionSpace( csName,
                new BasicBSONObject( "Domain", domainName ) );
        cl = cs.createCollection( clName );
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCSThread renameThread = new RenameCSThread();
        RemoveDomainThread removeThread = new RemoveDomainThread();

        renameThread.start();
        removeThread.start();

        Assert.assertTrue( renameThread.isSuccess(),
                renameThread.getErrorMsg() );

        if ( !removeThread.isSuccess() ) {
            Integer[] errnosB = { -34, -147 };
            BaseException errorB = ( BaseException ) removeThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( removeThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCSResult( db, csName, newCSName, 1 );
            if ( removeThread.isSuccess() ) {
                Domain domain = db.getDomain( domainName );
                DBCursor cur = domain.listCSInDomain();
                while ( cur.hasNext() ) {
                    BSONObject obj = cur.getNext();
                    String csn = ( String ) obj.get( "Name" );
                    if ( csn.equals( csName ) || csn.equals( newCSName ) ) {
                        Assert.fail(
                                "cs it's been remove, It shouldn't exist" );
                    }
                }
            } else {
                Domain domain = db.getDomain( domainName );
                DBCursor cur = domain.listCSInDomain();
                while ( cur.hasNext() ) {
                    BSONObject obj = cur.getNext();
                    String csn = ( String ) obj.get( "Name" );
                    if ( !csn.equals( newCSName ) ) {
                        Assert.fail( "cs remove faild, rename success exp: "
                                + newCSName + " act: " + csn );
                    }
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, newCSName );
        } finally {
            sdb.dropDomain( domainName );
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    private class RemoveDomainThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace sdbcs = db.getCollectionSpace( csName );
                sdbcs.removeDomain();
            }
        }
    }

}
