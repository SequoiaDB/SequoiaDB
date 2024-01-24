package com.sequoias3.commlibs3.s3utils.bean;

import com.sequoiadb.commlib.Ssh;
import com.sequoias3.commlibs3.S3TestBase;

/**
 * Created by fanyu on 2019/6/6.
 */
public class S3NodeWrapper {
    public enum NodeStatus {
        STOP_SUCCESS, STOP_FAILURE, START_SUCCESS, START_FAILURE
    };

    private String hostname;
    private String port;
    private String username;
    private String password;
    private NodeStatus status;
    private String dbInstallDir;

    public S3NodeWrapper() throws Exception {
        this.hostname = S3TestBase.s3HostName;
        this.port = S3TestBase.s3Port;
        this.username = S3TestBase.remoteUser;
        this.password = S3TestBase.remotePwd;
        this.dbInstallDir = getDBInstallDir();
    }

    public S3NodeWrapper( String hostname, String port, String username,
            String password ) throws Exception {
        this.hostname = hostname;
        this.port = port;
        this.username = username;
        this.password = password;
        this.dbInstallDir = getDBInstallDir();
    }

    public boolean start() throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( this.hostname, this.username, this.password );
            ssh.exec( "source /etc/profile;" + this.dbInstallDir
                    + "/tools/sequoias3/sequoias3.sh start "
                    + ">/tmp/s3start.log" );
            if ( ssh.getExitStatus() == 0 ) {
                this.status = NodeStatus.START_SUCCESS;
            } else {
                this.status = NodeStatus.START_FAILURE;
                throw new Exception( "exec command : source /etc/profile;"
                        + this.dbInstallDir
                        + "/tools/sequoias3/sequoias3.sh start"
                        + "failed,stout= " + ssh.getStdout() );
            }
            System.out.println( "---start status=" + status );
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
        return true;
    }

    public boolean stop() throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( this.hostname, this.username, this.password );
            ssh.exec( this.dbInstallDir
                    + "/tools/sequoias3/sequoias3.sh stop -a" );
            if ( ssh.getExitStatus() == 0 ) {
                this.status = NodeStatus.STOP_SUCCESS;
            } else {
                this.status = NodeStatus.STOP_FAILURE;
                throw new Exception( "exec command : " + this.dbInstallDir
                        + "/tools/sequoias3/sequoias3.sh stop -a "
                        + "failed,stdout= " + ssh.getStdout() + ",stderr = "
                        + ssh.getStderr() );
            }
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
        return true;
    }

    public boolean isNodeActive() throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( this.hostname, this.username, this.password );
            ssh.exec( this.dbInstallDir
                    + "/tools/sequoias3/sequoias3.sh status" );
            String stdout;
            if ( ssh.getExitStatus() == 0 ) {
                stdout = ssh.getStdout();
                if ( stdout.contains( "sequoias3" ) ) {
                    return true;
                } else {
                    return false;
                }
            } else {
                throw new Exception( "exec command : " + this.dbInstallDir
                        + "/tools/sequoias3/sequoias3.sh status "
                        + "failed,stdout= " + ssh.getStdout() + ",stderr = "
                        + ssh.getStderr() );
            }
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
    }

    private String getDBInstallDir() throws Exception {
        Ssh ssh = new Ssh( this.hostname, this.username, this.password );
        String dir;
        try {
            ssh.exec( "cat /etc/default/sequoiadb |grep INSTALL_DIR" );
            String str = ssh.getStdout();
            if ( str.length() <= 0 ) {
                throw new Exception(
                        "exec command:cat /etc/default/sequoiadb |grep INSTALL_DIR can not find sequoiacm install dir" );
            }
            dir = str.substring( str.indexOf( "=" ) + 1, str.length() - 1 );
        } finally {
            ssh.disconnect();
        }
        return dir;
    }

    @Override
    public String toString() {
        return "[ s3hostname" + this.hostname + ",s3port = " + this.port
                + ",username = " + this.username + ",password = "
                + this.password + "]";
    }
}
