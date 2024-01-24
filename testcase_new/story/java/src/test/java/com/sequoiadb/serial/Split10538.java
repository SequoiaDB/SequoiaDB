package com.sequoiadb.serial;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
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
 * @FileName:SEQDB-10538 切分过程中执行数据备份 1、向cl中插入数据记录 2、执行split，设置切分条件
 *                       3、切分过程中执行数据备份，验证离线备份（分别备份源组和目标组数据）4、查看切分和数据备份结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10538 extends SdbTestBase {
    private String clName = "testcaseCL10538";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {

        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            List< String > groupsName = commlib.getDataGroupNames( commSdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );

            CollectionSpace customCS = commSdb.getCollectionSpace( csName );
            customCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData();// 写入待切分的记录（1000）
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    public void insertData() {
        try {
            DBCollection cl = commSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            for ( int i = 0; i < 1000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{sk:" + i + ",index:" + i + "}" );
                insertedData.add( obj );
            }
            cl.bulkInsert( insertedData, 0 );

        } catch ( BaseException e ) {
            throw e;
        }

    }

    @Test
    public void testBackup() {
        Sequoiadb db = null;
        Split splitThread = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            splitThread.start();
            // 备份源组和目标组
            db = new Sequoiadb( coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            try {
                Thread.sleep( 1500 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            db.backupOffline( ( BSONObject ) JSON
                    .parse( "{Name:'testcase10538backupSrc',GroupName:'"
                            + srcGroupName + "'}" ) );
            checkBackup( db, "testcase10538backupSrc", 1 );
            db.backupOffline( ( BSONObject ) JSON
                    .parse( "{Name:'testcase10538backupDest',GroupName:'"
                            + destGroupName + "'}" ) );
            checkBackup( db, "testcase10538backupDest", 1 );

            // 等待切分结束,检查源和目标
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
            // Assert.assertEquals(backupThread.isSuccess(), true,
            // backupThread.getErrorMsg());
            checkDestGroup( 900, "{sk:{$gte:100,$lt:1000}}", 900,
                    destGroupName );
            checkDestGroup( 100, "{sk:{$gte:0,$lt:100}}", 100, srcGroupName );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
            if ( splitThread != null ) {
                splitThread.join();
            }
        }
    }

    private void checkBackup( Sequoiadb db, String backupName, int backupNum ) {
        DBCursor cursor = null;
        try {
            cursor = db.listBackup( null,
                    ( BSONObject ) JSON.parse( "{Name:'" + backupName + "'}" ),
                    null, null );
            int actualBackupNum = 0;
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                // circumvent the problem of null pointers to list does not
                // obtain correct info
                if ( !obj.containsField( "HasError" ) ) {
                    Assert.fail( "list info exist error:" + obj );
                }
                boolean flag = ( boolean ) obj.get( "HasError" );
                if ( flag ) {
                    Assert.fail( "backup has error:" + obj );
                }
                actualBackupNum++;
            }
            Assert.assertEquals( actualBackupNum, backupNum,
                    "backupNum wrong" );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }

    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            CollectionSpace cs = commSdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
            commSdb.removeBackup( ( BSONObject ) JSON
                    .parse( "{Name:'testcase10538backupSrc'}" ) );
            commSdb.removeBackup( ( BSONObject ) JSON
                    .parse( "{Name:'testcase10538backupDest'}" ) );

        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    private void checkDestGroup( int expectedCount, String macher,
            int expectTotalCount, String groupName ) {
        Sequoiadb destDataNode = null;
        try {
            destDataNode = commSdb.getReplicaGroup( groupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            DBCollection destCL = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            long count = destCL.getCount( macher );
            Assert.assertEquals( count, expectedCount );// 目标组应当含有上述查询数据
            Assert.assertEquals( destCL.getCount(), expectTotalCount ); // 目标组应当含有的数据量
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( destDataNode != null ) {
                destDataNode.disconnect();
            }
        }
    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:100}" ), // 切分
                        ( BSONObject ) JSON.parse( "{sk:1000}" ) );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }

        }

    }

}
