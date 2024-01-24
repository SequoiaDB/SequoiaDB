package com.sequoiadb.subcl.diskfull;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @FileName:SEQDB-2327 attachCL过程中catalog备节点所在服务器磁盘满
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class DiskFullSubcl2327 extends SdbTestBase {
    private String mainClName = "testcaseCL2327";
    private List< String > subClNames = new ArrayList< String >();
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private GroupMgr groupMgr = null;
    private Sequoiadb commSdb;
    private boolean clearFlag = false;
    private int bound = 0;
    private NodeWrapper cataSlave = null;
    private String pad_M;
    private String pad_K;
    private String pad_HK;
    private String pad_HM;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            cataSlave = groupMgr.getGroupByName( "SYSCatalogGroup" ).getSlave();
            // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
            if ( !groupMgr.checkBusiness( 20 ) ) {
                throw new SkipException( "checkBusiness return false" );
            }

            commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            commCS = commSdb.getCollectionSpace( csName );
            mainCL = commCS.createCollection( mainClName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{'sk':1},ShardingType:'range',IsMainCL:true}" ) );

            createSubCL( 500 );

            pad_M = Utils.getString( 1024 * 1024 );
            pad_HM = Utils.getString( 512 * 1024 );
            pad_K = Utils.getString( 1024 );
            pad_HK = Utils.getString( 512 );

        } catch ( ReliabilityException e ) {
            if ( commSdb != null ) {
                commSdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getStackString( e ) );
        }
    }

    private void fillUpCatalogSYSCL( String name, String padStr ) {
        Sequoiadb db = null;
        try {
            System.out.println( "strlen:" + padStr.length() );
            db = new Sequoiadb(
                    cataSlave.hostName() + ":" + cataSlave.svcName(), "", "" );
            DBCollection cl = db.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );
            int i = 0;
            try {
                while ( true ) {
                    cl.insert( "{Name:'" + name + i + "',pad:'" + padStr + i
                            + "',deleteFlag:1}" );
                    i++;
                }
            } catch ( BaseException e ) {
                System.out.println( "fillUpCataSYSCL:" + e.getErrorCode() );
                if ( e.getErrorCode() != -11 ) {
                    throw e;
                }
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void createSubCL( int subClCount ) {
        for ( int i = 0; i < subClCount; i++ ) {
            DBCollection cl = commCS
                    .createCollection( mainClName + "_sub_" + i );
            subClNames.add( cl.getName() );
        }
    }

    @Test
    public void test() throws Exception {
        // 磁盘满
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        System.out.println( "fillUpHost:" + cataSlave.hostName() );
        DiskFull df = new DiskFull( cataSlave.hostName(),
                SdbTestBase.reservedDir );
        df.init();
        df.make();

        // 分别以每条记录1m，512K,1k的大小填充SYSCAT.SYSCOLLECTIONS至-11错误
        fillUpCatalogSYSCL( "pad_M", pad_M );
        fillUpCatalogSYSCL( "pad_HM", pad_HM );
        fillUpCatalogSYSCL( "pad_K", pad_K );

        // 启动attach的线程，及填充SYSCAT.SYSCOLLECTIONS的线程（每条记录512字节）
        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new Attach() );
        mgr.addTask( new InsertToCataCL() );
        mgr.execute();

        // 检测线程执行结果
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // 磁盘恢复
        df.restore();
        df.fini();
        // cataLog恢复
        Sequoiadb cataDb = new Sequoiadb(
                cataSlave.hostName() + ":" + cataSlave.svcName(), "", "" );
        try {
            DBCollection catacl = cataDb.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );
            catacl.delete( "{deleteFlag:1}" );
        } catch ( BaseException e ) {
            cataDb.close();
        }

        // 检测CATALOG组数据一致
        Assert.assertEquals( cataGroup.checkInspect( 120 ), true );

        // 插入数据
        for ( int i = 0; i < bound; i++ ) {
            mainCL.insert( "{sk:" + i + "}" );
        }
        DBCursor cusor = mainCL.query( null, "{sk:1}", "{sk:1}", null );
        int count = 0;
        // 查询
        while ( cusor.hasNext() ) {
            Assert.assertEquals( cusor.getNext(),
                    ( BSONObject ) JSON.parse( "{sk:" + count + "}" ) );
            count++;
        }
        Assert.assertEquals( count, bound );

        clearFlag = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                for ( String subClName : subClNames ) {
                    commCS.dropCollection( subClName );
                }
                commCS.dropCollection( mainClName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n" + Utils.getStackString( e ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }

        }
    }

    class Attach extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            bound = 0;
            try {
                for ( String name : subClNames ) {
                    mainCL.attachCollection( csName + "." + name,
                            ( BSONObject ) JSON.parse(
                                    "{LowBound:{sk:" + bound + "},UpBound:{sk:"
                                            + ( bound + 100 ) + "}}" ) );
                    bound += 100;
                }
            } finally {
                System.out.println( "attach bound:" + bound );
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

    class InsertToCataCL extends OperateTask {
        @Override
        public void exec() throws Exception {
            fillUpCatalogSYSCL( "PAD", pad_HK );
        }

    }

}
