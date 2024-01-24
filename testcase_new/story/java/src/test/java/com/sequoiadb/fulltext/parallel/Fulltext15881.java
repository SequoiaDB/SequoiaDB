package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15881:truncate集合记录与alter集合并发
 * @Author huangxiaoni
 * @Date 2019.5.14
 */

public class Fulltext15881 extends FullTestBase {
    private Random random = new Random();
    private String groupName;
    private final String CL_NAME = "cl_es_15881";
    private final String IDX_NAME = "idx_es_15881";
    private final BSONObject IDX_KEY = new BasicBSONObject( "a", "text" );
    private final int RECS_NUM = 20000;

    private String cappedCSName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, CL_NAME );
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        String clOptions = new BasicBSONObject( "Group", groupName ).toString();
        caseProp.setProperty( CLOPT, clOptions );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( IDX_NAME, IDX_KEY, false, false );
        FullTextDBUtils.insertData( cl, RECS_NUM );
        cappedCSName = FullTextDBUtils.getCappedName( cl, IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, IDX_NAME );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadAlterCL threadAlterCL = new ThreadAlterCL();
        es.addWorker( threadTruncate );
        es.addWorker( threadAlterCL );
        es.run();

        // check results
        int cnt = ( int ) cl.getCount();
        if ( threadTruncate.getRetCode() == 0 ) {
            Assert.assertEquals( cnt, 0 );
        } else if ( threadTruncate.getRetCode() != 0 ) {
            Assert.assertEquals( cnt, RECS_NUM );
        }
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, IDX_NAME, cnt ) );

        if ( threadAlterCL.getRetCode() == 0 ) {
            this.checkAlterCLResults( true );
        } else if ( threadAlterCL.getRetCode() != 0 ) {
            this.checkAlterCLResults( false );
        }

        this.checkIndexCommitLSN( 300 );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            Thread.sleep( random.nextInt( 30 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -190 && e.getErrorCode() != -147 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }

    private class ThreadAlterCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void alterCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject options = new BasicBSONObject();
                options.put( "ShardingType", "hash" );
                options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.alterCollection( options );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -321 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }

    private void checkAlterCLResults( boolean alterCLSucc ) {
        DBCursor cursor = sdb.getSnapshot( 8,
                new BasicBSONObject( "Name", cl.getFullName() ), null, null );
        BasicBSONObject clInfo = ( BasicBSONObject ) cursor.getCurrent();
        if ( alterCLSucc ) {
            Assert.assertEquals( clInfo.get( "ShardingType" ).toString(),
                    "hash" );
        } else {
            Assert.assertFalse( clInfo.containsField( "ShardingType" ) );
        }
    }

    private void checkIndexCommitLSN( int checkTimes ) {
        for ( int i = 0; i < checkTimes; i++ ) {
            if ( checkIndexCommitLSN() ) {
                return;
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        Assert.fail( "Fulltext15881 checkIndexCommitLSN timeout..." );
    }

    private boolean checkIndexCommitLSN() {
        List< String > nodeAddress = CommLib.getNodeAddress( sdb, groupName );
        if ( nodeAddress.size() < 2 ) {
            return true;
        }

        List< Long > LSNs = new ArrayList<>();
        for ( String address : nodeAddress ) {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( address, "", "" );
                BSONObject match = new BasicBSONObject( "Name",
                        SdbTestBase.csName + "." + CL_NAME );
                DBCursor cursor = db.getSnapshot(
                        Sequoiadb.SDB_SNAP_COLLECTIONS, match, null, null );
                BSONObject info = cursor.getNext();
                BasicBSONList detail = ( BasicBSONList ) info.get( "Details" );
                long idxCommitLSN = ( long ) ( ( BasicBSONObject ) detail
                        .get( 0 ) ).get( "IndexCommitLSN" );
                LSNs.add( idxCommitLSN );
                cursor.close();
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }

        for ( int i = 0; i < LSNs.size(); i++ ) {
            if ( i > 0 ) {
                if ( !LSNs.get( i - 1 ).equals( LSNs.get( i ) ) ) {
                    System.out.println( "Fulltext15881: " + LSNs + " LSN["
                            + ( i - 1 ) + "] not equal LSN[" + i + "]" );
                    return false;
                }
            }
        }
        return true;
    }
}
