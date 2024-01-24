package com.sequoiadb.rename;

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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:TestRenameCS16138
 * @content alter集合空间属性和修改cs名并发
 * @author chensiqin
 * @Date 2018-10-20
 * @version 1.00
 */
public class TestRenameCS16138 extends SdbTestBase {

    private String csName = "cs16138";
    private String newCSName = "newcs16138";
    private String clName = "cl16138";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test16138() {
        RenameCSThread renameCSThread = new RenameCSThread();
        AlterCSThread alterCSThread = new AlterCSThread();
        renameCSThread.start();
        alterCSThread.start();

        if ( renameCSThread.isSuccess() && !alterCSThread.isSuccess() ) {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            BaseException e = ( BaseException ) alterCSThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -34 && e.getErrorCode() != -147
                    && e.getErrorCode() != -190 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !renameCSThread.isSuccess() && alterCSThread.isSuccess() ) {
            checkCSCataInfo( csName );
            BaseException e = ( BaseException ) renameCSThread.getExceptions()
                    .get( 0 );
            if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                Assert.fail( "errcode not expected : " + e.getMessage() );
            }
        } else if ( !renameCSThread.isSuccess()
                && !alterCSThread.isSuccess() ) {
            Assert.fail( "renameCSThread and alterCSThread all failed: "
                    + renameCSThread.getErrorMsg()
                    + alterCSThread.getErrorMsg() );
        } else { // 未撞到并发时
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, 1 );
            checkCSCataInfo( newCSName );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                sdb.dropCollectionSpace( newCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    public void checkCSCataInfo( String localCsName ) {
        DBQuery query = new DBQuery();
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        matcher.put( "Name", localCsName );
        ReplicaGroup cataRg = sdb.getReplicaGroup( "SYSCatalogGroup" );
        Sequoiadb cataDB = cataRg.getMaster().connect();
        CollectionSpace sysCS = cataDB.getCollectionSpace( "SYSCAT" );
        DBCollection sysCL = sysCS.getCollection( "SYSCOLLECTIONSPACES" );
        query.setMatcher( matcher );
        DBCursor cur = sysCL.query( query );
        Assert.assertTrue( cur.hasNext() );
        actual = cur.getNext();
        Assert.assertEquals( actual.get( "LobPageSize" ).toString(),
                8192 + "" );
        cur.close();
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                db.renameCollectionSpace( csName, newCSName );
            } finally {
                db.close();
            }
        }
    }

    private class AlterCSThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            try {
                CollectionSpace localcs = db.getCollectionSpace( csName );
                BSONObject alterList = new BasicBSONList();
                BSONObject alterBson = new BasicBSONObject();
                alterBson.put( "Name", "set attributes" );
                alterBson.put( "Args",
                        new BasicBSONObject( "LobPageSize", 8192 ) );
                alterList.put( Integer.toString( 0 ), alterBson );

                BSONObject options = new BasicBSONObject();
                options.put( "Alter", alterList );
                localcs.alterCollectionSpace( options );
            } finally {
                db.close();
            }
        }
    }

}
