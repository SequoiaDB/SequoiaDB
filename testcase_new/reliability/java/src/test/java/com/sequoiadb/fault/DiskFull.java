/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:DiskFull.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.fault;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.FaultMakeTask;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;

public class DiskFull extends Fault {
    private final static Logger log = Logger
            .getLogger( DiskFull.class.getName() );

    private String hostName;
    private String user;
    private String passwd;
    private String padPath;
    private int port;
    private List< String > padFileList = new ArrayList< String >();
    private Ssh ssh;
    private String remotePath;
    private int presetPercent;
    private final String localScriptPath = SdbTestBase.scriptDir;
    private final String scriptName = "fillUpDisk.sh";

    /**
     * @param hostName
     * @param padPath
     *            填充文件的路径
     */
    public DiskFull( String hostName, String padPath ) {
        super( "diskFull" );
        this.hostName = hostName;
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.padPath = padPath;
        this.remotePath = SdbTestBase.workDir;
        this.port = 22;
        this.presetPercent = 0;
    }

    public DiskFull( String hostName, String padPath, int presetPercent ) {
        super( "diskFull" );
        this.hostName = hostName;
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.padPath = padPath;
        this.remotePath = SdbTestBase.workDir;
        this.port = 22;
        this.presetPercent = presetPercent;
    }

    // for debug
    public void setPort( int port ) {
        this.port = port;
    }

    // for debug
    public void setUser( String user ) {
        this.user = user;
    }

    // for debug
    public void setPwd( String passwd ) {
        this.passwd = passwd;
    }

    // for debug
    public void setRemotePath( String remotePath ) {
        this.remotePath = remotePath;
    }

    @Override
    public void make() throws FaultException {
        log.info( "target path: " + hostName + " : " + padPath );
        fillUpDisk( 100 );
    }

    @Override
    public boolean checkMakeResult() throws FaultException {
        try {
            ssh.exec( "df " + padPath + " | sed '1d' |awk '{print $4}'" );
            String stdOut = ssh.getStdout();
            if ( stdOut.length() <= 0 ) {
                throw new FaultException( "can not find this path:" + padPath );
            }
            int padSize = Integer
                    .parseInt( stdOut.substring( 0, stdOut.length() - 1 ) );
            if ( padSize > 0 ) {
                this.fillUpDisk( 100 );
                return false;
            } else {
                return true;
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void restore() throws FaultException {
        for ( int i = 0; i < padFileList.size(); ) {
            try {
                ssh.exec( "rm -f " + padFileList.get( i ) );
            } catch ( ReliabilityException e ) {
                FaultException e1 = new FaultException( e );
                e1.setStackTrace( e.getStackTrace() );
                throw e1;
            }
            padFileList.remove( i );
        }
    }

    @Override
    public boolean checkRestoreResult() {
        return padFileList.size() == 0 ? true : false;
    }

    public void fillUpDisk( int percent ) throws FaultException {
        try {
            ssh.exec( remotePath + "/" + scriptName + " " + padPath + " "
                    + percent );
            if ( ssh.getStdout().length() > 0
                    && !ssh.getStdout().equals( "NothingToDo\n" ) ) {
                String padFileName = ssh.getStdout().substring( 0,
                        ssh.getStdout().length() - 1 );
                padFileList.add( padFileName );
            }
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
            fillUpDisk( presetPercent );
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
                ssh = null;
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    /**
     * @param hostName
     * @param padPath
     *            填充路徑
     * @param maxDelay
     *            最大延迟启动时间s
     * @param duration
     *            持续时间s
     * @param presetPercent
     *            线程启动之前，希望磁盘占用率达到的数值(<98)
     * @param sysCataCL
     *            待填满的系统编目表（SYSCOLLECTIONS,SYSDOMAINS...）
     *            备注：当填充catalog节点时，单纯通过创建domain/cs/cl增加编目信息很难填满磁盘，
     *            如果填写该参数，则可以通过特意插入记录来填满该编目表的空间。 缺省，则不做此特殊处理。
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( String hostName,
            String padPath, int maxDelay, int duration, DBCollection sysCataCL,
            int presetPercent ) {
        FaultMakeTask task = null;
        if ( sysCataCL != null ) {
            DiskFullForCata df = new DiskFullForCata( hostName, padPath,
                    presetPercent, sysCataCL );
            task = new FaultMakeTask( df, maxDelay, duration, 3 );
        } else {
            DiskFull df = new DiskFull( hostName, padPath, presetPercent );
            task = new FaultMakeTask( df, maxDelay, duration, 3 );
        }
        return task;
    }

    public static FaultMakeTask getFaultMakeTask( String hostName,
            String padPath, int maxDelay, int duration,
            DBCollection sysCataCL ) {
        int defaultPercent = 97;
        return getFaultMakeTask( hostName, padPath, maxDelay, duration,
                sysCataCL, defaultPercent );
    }

    public static FaultMakeTask getFaultMakeTask( String hostName,
            String padPath, int maxDelay, int duration ) {
        return getFaultMakeTask( hostName, padPath, maxDelay, duration, null );
    }
}
