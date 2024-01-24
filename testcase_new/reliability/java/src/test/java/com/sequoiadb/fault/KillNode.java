package com.sequoiadb.fault;

import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.FaultMakeTask;

import java.util.logging.Logger;

public class KillNode extends Fault {
    private String hostName;
    private String svcName;
    private String user;
    private String passwd;
    private String pid = "-1";
    private Ssh ssh;
    private String remotePath;
    private int port;
    private final String localScriptPath = SdbTestBase.scriptDir;
    private final String scriptName = "killNode.sh";
    private final static Logger log = Logger
            .getLogger( KillNode.class.getName() );

    public KillNode( String hostName, String svcName ) {
        super( "killNode" );
        this.hostName = hostName;
        this.svcName = svcName;
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.remotePath = SdbTestBase.workDir;
        this.port = 22;
    }

    @Override
    public void make() throws FaultException {
        log.info( "target node:" + hostName + " : " + svcName );
        try {
            ssh.exec( remotePath + "/" + scriptName + " " + svcName );
            pid = ssh.getStdout().substring( 0, ssh.getStdout().length() - 1 );
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public boolean checkMakeResult() throws FaultException {
        if ( pid.equals( "-1" ) ) {
            return false;
        }
        try {
            ssh.exec( "lsof -i:" + svcName + " | sed '1d' | awk '{print $2}'" );
            if ( ssh.getStdout().length() <= 0 ) {
                return false;
            }
            String currentPid = ssh.getStdout().substring( 0,
                    ssh.getStdout().length() - 1 );
            if ( !pid.equals( currentPid ) ) {
                pid = currentPid;
                return true;
            } else {
                return false;
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void restore() throws FaultException {

        // nothing to do
    }

    @Override
    public boolean checkRestoreResult() throws FaultException {
        try {
            ssh.exec( "lsof -i:" + svcName + " | sed '1d' | awk '{print $2}'" );
            if ( ssh.getStdout().length() <= 0 ) {
                return false;
            }
            return true;
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void init() throws FaultException {
        try {
            ssh = new Ssh( hostName, user, passwd, port );

            ssh.scpTo( localScriptPath + "/" + scriptName, remotePath + "/" );
            ssh.exec( "chmod 777 " + remotePath + "/" + scriptName );
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void fini() throws FaultException {
        try {
            if ( ssh != null ) {
                ssh.exec( "rm -rf " + remotePath + "/" + scriptName );
                ssh.disconnect();
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    /**
     * @param hostName
     * @param svcName
     * @param maxDelay
     *            最大延迟启动时间s
     * @param checkTimes
     *            构造成功与否的检查次数（20）
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( String hostName,
            String svcName, int maxDelay, int checkTimes ) {
        FaultMakeTask task = null;
        KillNode kn = new KillNode( hostName, svcName );
        task = new FaultMakeTask( kn, maxDelay, 3, checkTimes );
        return task;
    }

    public static FaultMakeTask getFaultMakeTask( NodeWrapper node,
            int maxDelay ) {
        return getFaultMakeTask( node.hostName(), node.svcName(), maxDelay );
    }

    /**
     * @param hostName
     * @param svcName
     * @param maxDelay
     *            最大延迟启动时间s
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( String hostName,
            String svcName, int maxDelay ) {
        FaultMakeTask task = null;
        KillNode kn = new KillNode( hostName, svcName );
        task = new FaultMakeTask( kn, maxDelay, 3, 1000 );// TODO:1000
                                                          // checkTimes
                                                          // jira:2383
        return task;
    }
}
