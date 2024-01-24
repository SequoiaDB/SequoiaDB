package com.sequoiadb.fault;

import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.task.FaultMakeTask;

public class KillSdbseadapter extends FaultFulltextBase {
    private String sdbseadapterDir;

    protected KillSdbseadapter( String hostName, String svcName ) {
        super( "KillSdbseadapter" );
        this.hostName = hostName;
        this.svcName = svcName.substring( 0, svcName.length() - 1 );
        user = "root";
        password = SdbTestBase.rootPwd;
        port = 22;
        localPath = SdbTestBase.scriptDir;
        remotePath = SdbTestBase.workDir;
        progName = "sdbseadapter";
        killRestart = "-9";
        sdbseadapterDir = SdbTestBase.sdbseadapterDir;
    }

    @Override
    protected void getMakeStdout() {
        super.getMakeStdout();
        String stdout = ssh.getStdout().trim();
        pid = stdout.split( ":" )[ 0 ];
        cmdDir = stdout.split( ":" )[ 1 ];
    }

    @Override
    protected void beforeRestore() {
        super.beforeRestore();
        cmdArgs = setRestoreArgs( progName, cmdDir, sdbseadapterDir,
                svcName + "0" );
    }

    /**
     * 默认持续时间 3s
     * 
     * @param hostName
     *            主机名
     * @param svcName
     *            适配器对应的节点的 svcname
     * @param maxDlay
     *            最大延迟启动时间，单位：s
     * @param checkTimes
     *            构造成功与否检查次数，每次检查完后 sleep 0.5s
     * @return
     */
    public static FaultMakeTask geFaultMakeTask( String hostName,
            String svcName, int maxDlay, int checkTimes ) {
        FaultMakeTask task = null;
        KillSdbseadapter ks = new KillSdbseadapter( hostName, svcName );
        task = new FaultMakeTask( ks, maxDlay, 3, checkTimes );
        return task;
    }

    /**
     * 默认检查次数 120 次，持续时间 3s
     * 
     * @param hostName
     *            主机名
     * @param svcName
     *            适配器对应的节点的 svcname
     * @param maxDelay
     *            最大延迟启动时间，单位：s
     * @return
     */
    public static FaultMakeTask geFaultMakeTask( String hostName,
            String svcName, int maxDelay ) {
        return geFaultMakeTask( hostName, svcName, maxDelay, 120 );
    }
}
