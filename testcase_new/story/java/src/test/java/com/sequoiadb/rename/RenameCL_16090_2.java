package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCL_16090_2.java 并发索引操作和修改cl名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16090_2 extends SdbTestBase {

    private String csName = "renameCS_16090_2";
    private String clName = "rename_CL_16090_2";
    private String newCLName = "rename_CL_16090_new_2";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 1000;
    private String indexNameA = "index_16090A_2";
    private String indexNameB = "index_16090B_2";
    private int createTimes = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            CommLib.clearCS( sdb, csName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName,
                new BasicBSONObject( "ReplSize", 0 ) );
        for ( int i = 0; i < 10; i++ ) {
            cl.createIndex( indexNameA + "_" + i,
                    new BasicBSONObject( "a" + i, 1 ), false, false );
        }
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        CreateIndexThread createThread = new CreateIndexThread();

        renameCLThread.start();
        createThread.start();

        boolean rename = renameCLThread.isSuccess();
        boolean create = createThread.isSuccess();
        Assert.assertTrue( rename, renameCLThread.getErrorMsg() );

        if ( !create ) {
            Integer[] errnos = { -23, -147, -190 };
            BaseException error = ( BaseException ) createThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( createThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            checkCreateIndex( db, csName, newCLName, create );

        }

    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( new Random().nextInt( 300 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
                if ( !CommLib.isStandAlone( db ) ) {
                    checkCLRename( db, newCLName );
                }
            }
        }
    }

    private class CreateIndexThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    cl.createIndex( indexNameB + "_" + i,
                            new BasicBSONObject( "b" + i, 1 ), false, false );
                    createTimes++;
                }
            }
        }
    }

    private void checkCreateIndex( Sequoiadb db, String csName, String clName,
            boolean success ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cur = cl.getIndexes();
        List< String > indexNames = new ArrayList< String >();
        int indexAnum = 0;
        try {
            while ( cur.hasNext() ) {
                BSONObject obj = cur.getNext();
                BSONObject indexInfo = ( BSONObject ) obj.get( "IndexDef" );
                String name = ( String ) indexInfo.get( "name" );
                indexNames.add( name );
                if ( name.indexOf( indexNameA ) >= 0 ) {
                    indexAnum++;
                }
            }
        } finally {
            if ( cur != null ) {
                cur.close();
            }
        }
        Assert.assertEquals( indexAnum, 10, "check indexA num" );

        if ( success ) {
            Assert.assertEquals( indexNames.size(), 21,
                    "check sum indexA and indexB num" );
        } else {
            int leftNum = 0;
            for ( String indexName : indexNames ) {
                if ( indexName.indexOf( indexNameB ) != -1 ) {
                    leftNum++;
                }
            }
            if ( leftNum != createTimes ) {
                Assert.fail( "check indexB num error, exp: " + createTimes
                        + " act: " + leftNum );
            }
        }
    }

    /**
     * @Description：直连数据节点检查复制组内修改clname是否成功
     * @author: zhaohailin
     * @param sdb
     * @param newCLName
     */
    private void checkCLRename( Sequoiadb sdb, String newCLName ) {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( newCLName );
        List< String > clGroups = CommLib.getCLGroups( cl );
        for ( String groupname : clGroups ) {
            int successNodeNum = 0;
            List< String > nodeAddrs = CommLib.getNodeAddress( sdb, groupname );
            for ( String nodeAddr : nodeAddrs ) {
                try ( Sequoiadb dataDB = new Sequoiadb( nodeAddr, "", "" )) {
                    CollectionSpace dataCl = dataDB
                            .getCollectionSpace( csName );
                    if ( dataCl.isCollectionExist( newCLName ) ) {
                        successNodeNum++;
                    }
                }
            }
            if ( successNodeNum < ( nodeAddrs.size() / 2 + 1 ) ) {
                Assert.fail(
                        "check clname error, exp:successNodeNum not more than a half.  act : successNodeNum ="
                                + successNodeNum );
            }
        }
    }
}
