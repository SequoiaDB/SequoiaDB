package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static org.testng.Assert.assertTrue;

/**
 * 创建domain时catalog备节点正常重启
 * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
 * 2、创建domian时catalog备节点正常重启（如执行kill -15杀掉节点进程，构造节点正常重启）
 * 3、查看domain创建结果和catalog主节点状态
 * 4、节点启动成功后（查看节点进程存在）
 * 5、再次创建其他domain，并指定该domain创建CS
 * 6、查看domain创建结果（执行db.listDomain命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该domain相关信息
 */

/**
 * @FileName seqDB-2285
 * @Author laojingtang
 * @Date 17-4-20
 * @Version 1.00
 */
public class CreateDomain2285 extends SdbTestBase
        implements StandTestInterface {
    private List< String > domainNames = new ArrayList<>( 100 );
    private Sequoiadb db;

    @BeforeClass
    public void setup() {
        MyUtil.printBeginTime( this );
        db = MyUtil.getSdb();
        for ( int i = 0; i < 100; i++ ) {
            domainNames.add( "domain" + i );
        }
    }

    @Test
    void test() throws ReliabilityException {
        FaultMakeTask faultMakeTask = com.sequoiadb.fault.NodeRestart
                .getFaultMakeTask(
                        GroupMgr.getInstance()
                                .getGroupByName( "SYSCatalogGroup" ).getSlave(),
                        1, 10 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        OperateTask task = DBoperateTask.getTaskCreateDomains( domainNames );
        taskMgr.addTask( task );
        taskMgr.addTask( new OperateTask() {
            @Override
            public void exec() throws Exception {

            }
        } );
        taskMgr.execute();
        // 检查并发结果是否正确

        // 检查domain是否正确创建
        assertTrue( MyUtil.isDomainAllCreated( domainNames ) );
        assertTrue( MyUtil.isCatalogGroupSync() );

        // 再次创建domian和cs
        BSONObject options = ( BSONObject ) JSON
                .parse( "{'Groups':['group1']}" );
        db.createDomain( "csdomain", options );
        db.createCollectionSpace( "cs2285",
                new BasicBSONObject( "Domain", "csdomain" ) );
        Object obj = db.listDomains( new BasicBSONObject( "Name", "csdomain" ),
                null, null, null );
        assertTrue( obj != null );
        DBCursor c = db.listCollectionSpaces();
        boolean flag = false;
        while ( c.hasNext() ) {
            if ( c.getNext().get( "Name" ).equals( "cs2285" ) )
                flag = true;
        }
        assertTrue( flag );
        assertTrue( MyUtil.isCatalogGroupSync() );
    }

    @AfterClass
    public void tearDown() {
        for ( String name : domainNames ) {
            db.dropDomain( name );
        }
        db.dropCollectionSpace( "cs2285" );
        db.dropDomain( "csdomain" );
        MyUtil.printEndTime( this );
    }
}
