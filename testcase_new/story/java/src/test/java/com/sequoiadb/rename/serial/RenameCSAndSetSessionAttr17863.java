package com.sequoiadb.rename.serial;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.rename.RenameUtil;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
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
 * @FileName RenameCSAndSetSessionAttr17863
 * @content set seesionAttr is "{PreferedInstance:'s'}",priority slave node
 *          query,the slave node has no synchronous rename CS *
 * @testlink seqDB-17863
 * @author wuyan
 * @Date 2019.2.18
 * @version 1.00
 */
public class RenameCSAndSetSessionAttr17863 extends SdbTestBase {
    private String clName = "renameCL_17863";
    private String oldCSName = "oldRenameCS_17863";
    private String newCSName = "newRenameCS_17863";
    private Sequoiadb sdb = null;
    private Sequoiadb sessionSdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private int recordNums = 300000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sessionSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = RenameUtil.getGroupName( sdb );
        if ( RenameUtil.getNodeNum( sdb, groupName ) < 2 ) {
            throw new SkipException(
                    "only one node in the group skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( oldCSName ) ) {
            sdb.dropCollectionSpace( oldCSName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }
        String options = "{ShardingKey:{no:1},ReplSize:1,Group:'" + groupName
                + "'}";
        cs = RenameUtil.createCS( sdb, oldCSName );
        RenameUtil.createCL( cs, clName, options );
        BSONObject session = new BasicBSONObject();
        session.put( "PreferedInstance", "s" );
        sessionSdb.setSessionAttr( session );
    }

    @Test
    public void test() {
        // concurrent insert,construction of slave node synchronization can not
        // keey up with the master node
        List< InsertThread > insertThreads = new ArrayList<>();
        int beginNo = 0;
        int endNo = 10000;
        int numsPerBatch = 10000;
        for ( int i = 0; i < recordNums / numsPerBatch; i++ ) {
            insertThreads.add( new InsertThread( beginNo, endNo ) );
            beginNo = endNo;
            endNo = beginNo + 10000;
        }
        for ( InsertThread insertThread : insertThreads ) {
            insertThread.start();
        }
        for ( InsertThread insertThread : insertThreads ) {
            Assert.assertTrue( insertThread.isSuccess(),
                    insertThread.getErrorMsg() );
        }

        // the slave node has no synchronous rename CS, query of new csname must
        // be access master node
        sdb.renameCollectionSpace( oldCSName, newCSName );
        String accessNodeAfterRename = getAccessNode( sessionSdb, newCSName );

        ReplicaGroup rg = sdb.getReplicaGroup( groupName );
        String masterNodeName = rg.getMaster().getNodeName();

        // if has sysnchronous rename CL, check the count records from slave
        // node.
        if ( accessNodeAfterRename != masterNodeName ) {
            long getCount = sessionSdb.getCollectionSpace( newCSName )
                    .getCollection( clName ).getCount();
            Assert.assertEquals( getCount, recordNums,
                    "accessNode is " + accessNodeAfterRename
                            + "  masterNode is " + masterNodeName
                            + " getCount is " + getCount );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( newCSName );
        } finally {
            if ( sdb != null )
                sdb.close();
            if ( sessionSdb != null ) {
                sessionSdb.close();
            }
        }
    }

    public class InsertThread extends SdbThreadBase {
        private int beginNo;
        private int endNo;

        public InsertThread( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( oldCSName )
                        .getCollection( clName );
                insertDatas( dbcl, endNo - beginNo, beginNo );
            }
        }
    }

    private String getAccessNode( Sequoiadb sdb, String csName ) {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cursor = dbcl.explain( null, null, null, null, 0, -1,
                DBQuery.FLG_QUERY_FORCE_HINT, null );
        String nodeName = "";
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            nodeName = ( String ) obj.get( "NodeName" );
        }
        cursor.close();
        return nodeName;
    }

    private void insertDatas( DBCollection dbcl, int insertNums, int beginNo ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNums = 10000;
        for ( int i = 0; i < batchNums; i++ ) {
            int count = beginNo++;
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", "test" + count );
            String str = "32345.06789123456" + count;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimala", decimal );
            obj.put( "no", count );
            obj.put( "order", count );
            obj.put( "inta", count );
            obj.put( "ftest", count + 0.2345 );
            obj.put( "str", "test_" + String.valueOf( count ) );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        insertRecord = null;
    }
}
