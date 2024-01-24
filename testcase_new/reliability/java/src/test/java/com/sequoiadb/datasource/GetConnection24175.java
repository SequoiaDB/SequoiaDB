package com.sequoiadb.datasource;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @Descreption seqDB-24175:创建连接池，断网后获取连接
 * @Author YiPan
 * @Date 2021.05.15
 */
public class GetConnection24175 extends SdbTestBase {

    public static SequoiadbDatasource ds;

    @BeforeClass
    private void setUp() throws IOException {
        List< String > connList = new ArrayList<>();
        connList.add( hostName + ":" + serviceName );
        ds = new SequoiadbDatasource( connList, remoteUser, remotePwd, null,
                null );
    }

    @Test
    public void test() throws Exception {
        // disalbe 连接池
        ds.disableDatasource();
        // 断网
        TaskMgr taskMgr = new TaskMgr();
        FaultMakeTask task = BrokenNetwork
                .getFaultMakeTask( SdbTestBase.hostName, 0, 10 );
        taskMgr.addTask( task );
        taskMgr.addTask( new GetConn() );
        taskMgr.execute();
        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
    }

    @AfterClass
    private void tearDown() throws Exception {
        ds.close();
    }

    private class GetConn extends OperateTask {

        @Override
        public void exec() throws Exception {
            Sequoiadb tmp = null;
            try {
                Random random = new Random();
                Thread.sleep( random.nextInt( 3000 ) );
                tmp = ds.getConnection( 2000 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -15 ) {
                    throw e;
                }
            } finally {
                if ( tmp != null ) {
                    ds.releaseConnection( tmp );
                }
            }
        }
    }

}
