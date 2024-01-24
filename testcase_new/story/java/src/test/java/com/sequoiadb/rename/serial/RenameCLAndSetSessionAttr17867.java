package com.sequoiadb.rename.serial;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;

import com.sequoiadb.rename.RenameUtil;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName RenameCLAndSetSessionAttr17867
 * @content set seesionAttr is "{PreferedInstance:'s'}",priority slave node
 *          query, do the following:count/getIndex/listIndex/getLob/listLobs,the
 *          slave node has no synchronous rename CL,the operator access the
 *          master node. *
 * @testlink seqDB-17867
 * @author wuyan
 * @Date 2019.2.18
 * @version 1.00
 */
public class RenameCLAndSetSessionAttr17867 extends SdbTestBase {
    private String csName = "renameCL_17867";
    private String oldCLName = "oldRenameCL_17867";
    private String newCLName = "newRenameCL_17867";
    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private Sequoiadb querySessionSdb = null;
    private Sequoiadb getCountSessionSdb = null;
    private Sequoiadb getIndexSessionSdb = null;
    private Sequoiadb listIndexSessionSdb = null;
    private Sequoiadb getLobSessionSdb = null;
    private Sequoiadb listLobSessionSdb = null;
    private String groupName = "";
    private CollectionSpace cs = null;
    private int recordNums = 200000;
    private ObjectId lobOid = new ObjectId( "5c6a5aa3d5d5a3f1282f66d0" );
    private byte[] lobBuff = new byte[ 1024 * 1024 ];

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase" );
        }

        groupName = RenameUtil.getGroupName( sdb );
        if ( RenameUtil.getNodeNum( sdb, groupName ) < 2 ) {
            throw new SkipException(
                    "only one node in the group skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        String options = "{ReplSize:1,Group:'" + groupName + "'}";
        cs = RenameUtil.createCS( sdb, csName );
        dbcl = RenameUtil.createCL( cs, oldCLName, options );
        dbcl.createIndex( "testa", "{inta:1,no:1}", true, false );
        dbcl.createIndex( "testb", "{str:1,no:1}", true, false );
        querySessionSdb = buildSdbAndSetSessionAttr( querySessionSdb );
        getCountSessionSdb = buildSdbAndSetSessionAttr( getCountSessionSdb );
        listIndexSessionSdb = buildSdbAndSetSessionAttr( listIndexSessionSdb );
        getIndexSessionSdb = buildSdbAndSetSessionAttr( getIndexSessionSdb );
        getLobSessionSdb = buildSdbAndSetSessionAttr( getLobSessionSdb );
        listLobSessionSdb = buildSdbAndSetSessionAttr( listLobSessionSdb );
    }

    @Test
    public void testInsertAndRename() {
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

        // specifies that loboid write lob,for test get lob
        putLob( sdb, dbcl );

        // the slave node has no synchronous rename CL, query of new csname must
        // be access master node
        cs.renameCollection( oldCLName, newCLName );
    }

    @Test(dependsOnMethods = "testInsertAndRename")
    public void testQuery() {
        // only verify that the query operation from slave node is normal
        ListIndexesThread listIndexes = new ListIndexesThread(
                listIndexSessionSdb );
        GetIndexThread getIndex = new GetIndexThread( getIndexSessionSdb );
        CountThread getCount = new CountThread( getCountSessionSdb );
        GetLobThread getLobThread = new GetLobThread( getLobSessionSdb );
        ListLobThread listLobThread = new ListLobThread( listLobSessionSdb );
        listIndexes.start();
        getIndex.start();
        getCount.start();
        getLobThread.start();
        listLobThread.start();

        Assert.assertTrue( listIndexes.isSuccess(), listIndexes.getErrorMsg() );
        Assert.assertTrue( getIndex.isSuccess(), getIndex.getErrorMsg() );
        Assert.assertTrue( getCount.isSuccess(), getCount.getErrorMsg() );
        Assert.assertTrue( getLobThread.isSuccess(),
                getLobThread.getErrorMsg() );
        Assert.assertTrue( listLobThread.isSuccess(),
                listLobThread.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    private Sequoiadb buildSdbAndSetSessionAttr( Sequoiadb sdb ) {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        BSONObject session = new BasicBSONObject();
        session.put( "PreferedInstance", "S" );
        sdb.setSessionAttr( session );
        return sdb;
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
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( oldCLName );
                insertDatas( dbcl, endNo - beginNo, beginNo );
            }
        }
    }

    public class CountThread extends SdbThreadBase {
        private Sequoiadb db;

        public CountThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws BaseException {
            try {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( newCLName );
                long count = dbcl.getCount();
                Assert.assertEquals( count, recordNums );
            } finally {
                db.close();
            }
        }
    }

    public class GetLobThread extends SdbThreadBase {
        private Sequoiadb db;

        public GetLobThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws BaseException {
            try {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( newCLName );
                // read and check the lob data
                try ( DBLob rLob = dbcl.openLob( lobOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    Arrays.equals( rbuff, lobBuff );
                }
            } finally {
                db.close();
            }
        }
    }

    public class ListLobThread extends SdbThreadBase {
        private Sequoiadb db;

        public ListLobThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws BaseException {
            try {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( newCLName );
                DBCursor listCursor = dbcl.listLobs();
                int count = 0;
                while ( listCursor.hasNext() ) {
                    count++;
                    listCursor.getNext();
                }
                listCursor.close();
                Assert.assertEquals( count, 1 );
            } finally {
                db.close();
            }
        }
    }

    public class ListIndexesThread extends SdbThreadBase {
        private Sequoiadb db;

        public ListIndexesThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws BaseException {
            try {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( newCLName );
                DBCursor listCursor = dbcl.getIndexes();
                List< String > indexes = new ArrayList<>();
                while ( listCursor.hasNext() ) {
                    BSONObject object = listCursor.getNext();
                    BSONObject indexInfo = ( BSONObject ) object
                            .get( "IndexDef" );
                    String indexName = ( String ) indexInfo.get( "name" );
                    indexes.add( indexName );
                }
                listCursor.close();
                // check the list indexes result
                List< String > expIndexes = new ArrayList<>();
                expIndexes.add( "$id" );
                expIndexes.add( "testa" );
                expIndexes.add( "testb" );
                Collections.sort( indexes );
                Collections.sort( expIndexes );
                Assert.assertEquals( indexes, expIndexes,
                        "only 3 indexes exist in the current cl!" );
            } finally {
                db.close();
            }
        }
    }

    public class GetIndexThread extends SdbThreadBase {
        private Sequoiadb db;

        public GetIndexThread( Sequoiadb db ) {
            this.db = db;
        }

        @Override
        public void exec() throws BaseException {
            try {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( newCLName );
                String name = "testa";
                BSONObject indexInfo = ( BSONObject ) dbcl.getIndexInfo( name )
                        .get( "IndexDef" );
                String indexName = ( String ) indexInfo.get( "name" );
                if ( !name.equals( indexName ) ) {
                    System.out.println( "indexName=" + indexName );
                    Assert.fail( "get index name error!" );
                }
            } finally {
                db.close();
            }
        }
    }

    private void putLob( Sequoiadb sdb, DBCollection dbcl ) {
        Random random = new Random();
        random.nextBytes( lobBuff );
        try ( DBLob lob = dbcl.createLob( lobOid )) {
            lob.write( lobBuff );
        }
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
