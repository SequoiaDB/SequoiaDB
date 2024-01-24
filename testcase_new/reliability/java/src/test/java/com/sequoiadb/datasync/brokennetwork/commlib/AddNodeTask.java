package com.sequoiadb.datasync.brokennetwork.commlib;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;

public class AddNodeTask extends OperateTask {
    private String groupName = null;
    private String host = null;
    private int port;

    public AddNodeTask( String groupName, String host, int port ) {
        this.groupName = groupName;
        this.host = host;
        this.port = port;
    }

    @Override
    public void init() {
        // 为了避免节点启动前就已经断网，在启动任务前启动节点
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        ReplicaGroup randomGroup = db.getReplicaGroup( groupName );
        String nodePath = SdbTestBase.reservedDir + "/data/" + port;
        Node newNode = randomGroup.createNode( host, port, nodePath,
                ( BSONObject ) null );
        newNode.start();
        db.close();
    }

    @Override
    public void exec() throws Exception {
        // 同步正在后台进行...
    }
}