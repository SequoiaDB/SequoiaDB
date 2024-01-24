package com.sequoiadb.fault;

import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;

public class KillElasticSearch extends FaultFulltextBase {

    protected KillElasticSearch( String hostName ) {
        super( "KillElasticSearch" );
        this.hostName = hostName;
        this.svcName = SdbTestBase.esServiceName;
        user = SdbTestBase.remoteUser;
        password = SdbTestBase.remotePwd;
        port = 22;
        localPath = SdbTestBase.scriptDir;
        remotePath = SdbTestBase.workDir;
        progName = "elasticsearch";
        killRestart = "-9";
    }

    @Override
    protected void getMakeStdout() {
        super.getMakeStdout();
        String stdout = ssh.getStdout().trim();
        pid = stdout.split( ":" )[ 0 ];
        cmdDir = stdout.split( ":" )[ 1 ] + "/bin/elasticsearch";
    }

    @Override
    protected String beforeCheckMakeResult() {
        return "lsof -i:" + svcName + " | sed '1d' | awk '{print $2}'";
    }

    @Override
    protected void beforeRestore() {
        super.beforeRestore();
        cmdArgs = setRestoreArgs( progName, cmdDir );
    }

    @Override
    protected String beforeCheckRestoreResult() {
        return "lsof -i:" + svcName + " | sed '1d' | awk '{print $2}'";
    }

    /**
     * 默认持续时间 3s
     * 
     * @param hostName
     *            主机名
     * @param maxDlay
     *            最大延迟启动时间，单位：s
     * @param checkTimes
     *            构造成功与否检查次数，每次检查完后 sleep 0.5s
     * @return
     */
    public static FaultMakeTask geFaultMakeTask( String hostName, int maxDlay,
            int checkTimes ) {
        FaultMakeTask task = null;
        KillElasticSearch re = new KillElasticSearch( hostName );
        task = new FaultMakeTask( re, maxDlay, 3, checkTimes );
        return task;
    }

    /**
     * 默认检查次数 120 次，持续时间 3s
     * 
     * @param hostName
     *            主机名
     * @param maxDelay
     *            最大延迟启动时间，单位：s
     * @return
     */
    public static FaultMakeTask geFaultMakeTask( String hostName,
            int maxDelay ) {
        return geFaultMakeTask( hostName, maxDelay, 120 );
    }
}
