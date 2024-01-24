package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @author luweikang
 * @Description RenameCL_16090.java 并发删除索引操作和修改cl名
 * @date 2018年10月17日
 */
public class RenameCL_16090_1 extends SdbTestBase {

    private String csName = "renameCS_16090_1";
    private String clName = "rename_CL_16090_1";
    private String newCLName = "rename_CL_16090_1_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private int recordNum = 1000;
    private String srcGroupName;
    private String indexNameA = "index_16090A_1";
    private String indexNameB = "index_16090B_1";
    private int dropTimes = 10;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupNames.get( 0 );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            CommLib.clearCS( sdb, csName );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName, new BasicBSONObject( "ReplSize", 0 )
                .append( "Group", srcGroupName ) );
        for ( int i = 0; i < 10; i++ ) {
            cl.createIndex( indexNameA + "_" + i,
                    new BasicBSONObject( "a" + i, 1 ), false, false );
            cl.createIndex( indexNameB + "_" + i,
                    new BasicBSONObject( "b" + i, 1 ), false, false );
        }
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameCLThread = new RenameCLThread();
        DropIndexThread dropThread = new DropIndexThread();

        renameCLThread.start();
        dropThread.start();

        boolean rename = renameCLThread.isSuccess();
        boolean drop = dropThread.isSuccess();
        Assert.assertTrue( rename, renameCLThread.getErrorMsg() );

        if ( !drop ) {
            Integer[] errnos = { -23, -147, -190 };
            BaseException error = ( BaseException ) dropThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnos ).contains( error.getErrorCode() ) ) {
                Assert.fail( dropThread.getErrorMsg() );
            }
        }
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            checkDropIndex( db, csName, newCLName, drop );
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
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
                isLSNConsistency( db, srcGroupName );
                checkNodeStatus( srcGroupName );
                checkCLRename( db, newCLName );
            }
        }
    }

    private class DropIndexThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    cl.dropIndex( indexNameB + "_" + i );
                    dropTimes--;
                }
            }
        }
    }

    private void checkDropIndex( Sequoiadb db, String csName, String clName,
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
                if ( name.indexOf( indexNameA ) != -1 ) {
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
            for ( String indexName : indexNames ) {
                if ( indexName.indexOf( indexNameB ) != -1 ) {
                    Assert.fail(
                            "drop all indexB success, indexB should not exist: "
                                    + indexName );
                }
            }
        } else {
            int leftNum = 0;
            for ( String indexName : indexNames ) {
                if ( indexName.indexOf( indexNameB ) != -1 ) {
                    leftNum++;
                }
            }
            if ( leftNum < dropTimes - 1 ) {
                Assert.fail( "check indexB num error, exp: " + dropTimes
                        + "act: " + leftNum );
            }
        }
    }

    /**
     * @param sdb
     * @param newCLName
     * @Description：直连数据节点检查复制组内修改clname是否成功
     * @author: zhaohailin
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

    /**
     * 检查CL主备节点集合CompleteLSN一致 *
     *
     * @param db
     *            new db连接
     * @param groupName
     *            组名
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isLSNConsistency( Sequoiadb db, String groupName )
            throws Exception {
        boolean isConsistency = false;
        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        Node masterNode = rg.getMaster();
        try ( Sequoiadb masterSdb = new Sequoiadb(
                masterNode.getHostName() + ":" + masterNode.getPort(), "",
                "" )) {
            long completeLSN = -2;
            DBCursor cursor = masterSdb.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                    null, "{CompleteLSN: ''}", null );
            if ( cursor.hasNext() ) {
                BasicBSONObject snapshot = ( BasicBSONObject ) cursor.getNext();
                if ( snapshot.containsField( "CompleteLSN" ) ) {
                    completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                }
            } else {
                throw new Exception( masterSdb.getNodeName()
                        + " can't not find system snapshot" );
            }
            cursor.close();
            for ( String nodeName : nodeNames ) {
                if ( masterNode.getNodeName().equals( nodeName ) ) {
                    continue;
                }
                isConsistency = false;
                try ( Sequoiadb nodeConn = new Sequoiadb( nodeName, "", "" )) {
                    DBCursor cur = null;
                    long checkCompleteLSN = -3;
                    for ( int i = 0; i < 600; i++ ) {
                        cur = nodeConn.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                                null, "{CompleteLSN: ''}", null );
                        if ( cur.hasNext() ) {
                            BasicBSONObject checkSnapshot = ( BasicBSONObject ) cur
                                    .getNext();
                            if ( checkSnapshot
                                    .containsField( "CompleteLSN" ) ) {
                                checkCompleteLSN = ( long ) checkSnapshot
                                        .get( "CompleteLSN" );
                            }
                        }
                        cur.close();
                        if ( completeLSN <= checkCompleteLSN ) {
                            isConsistency = true;
                            break;
                        }
                        try {
                            Thread.sleep( 1000 );
                        } catch ( InterruptedException e ) {
                            e.printStackTrace();
                        }
                    }
                    if ( !isConsistency ) {
                        System.out.println( "Group [" + groupName
                                + "] node system snapshot is not the same, masterNode "
                                + masterNode.getNodeName() + " CompleteLSN: "
                                + completeLSN + ", " + nodeName
                                + " CompleteLSN: " + checkCompleteLSN );
                    }
                }
            }
        }
        return isConsistency;
    }

    /**
     * 检查指定节点的状态
     *
     * @param srcGroupName
     *            预期结果节点的状态
     * @return
     */
    public static void checkNodeStatus( String srcGroupName ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            int eachSleepTime = 1000;
            int doTimes = 0;
            int timeOut = 60000;
            String ftStatus = "";
            boolean isStatus = false;
            do {
                isStatus = true;
                DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                        new BasicBSONObject( "RawData", true )
                                .append( "GroupName", srcGroupName ),
                        new BasicBSONObject( "FTStatus", "" ), null );
                while ( cursor.hasNext() ) {
                    ftStatus = ( String ) cursor.getNext().get( "FTStatus" );
                    if ( !ftStatus.equals( "" ) ) {
                        isStatus = false;
                    }
                }
                cursor.close();
                if ( !isStatus ) {
                    doTimes += eachSleepTime;
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                }
            } while ( !isStatus && doTimes < timeOut );
            if ( doTimes >= timeOut ) {
                Assert.fail( "The expected ftStatus: " + ftStatus
                        + ", but actual ftStatus: " + ftStatus );
            }
        }
    }
}