/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:NodeRestart.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.fault;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.FaultMakeTask;

public class NodeRestart extends Fault {
    private NodeWrapper node;

    public NodeRestart( NodeWrapper node ) {
        super( "nodeRestart" );
        this.node = node;
    }

    @Override
    public void make() throws FaultException {
        System.out.println( "target node:" + this.node.hostName() + " : "
                + this.node.svcName() );
        try {
            this.node.stop();
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public boolean checkMakeResult() throws FaultException {
        Sequoiadb db = null;
        try {
            ConfigOptions conf = new ConfigOptions();
            conf.setConnectTimeout( 1000 );
            conf.setMaxAutoConnectRetryTime( 1000 );
            db = new Sequoiadb( node.hostName() + ":" + node.svcName(), "", "",
                    conf );
            return false;
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -15 ) {
                return true;
            } else {
                throw new FaultException( e );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Override
    public void restore() throws FaultException {
        try {
            this.node.start();
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public boolean checkRestoreResult() {
        return this.node.isNodeActive();
    }

    @Override
    public void init() throws FaultException {

    }

    @Override
    public void fini() throws FaultException {
    }

    /**
     * 
     * @param node
     * @param maxDelay
     *            最大延迟启动时间s
     * @param duration
     *            持续时间s
     * @param checkTimes
     *            检查构造成功与否的检测次数
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( NodeWrapper node,
            int maxDelay, int duration, int checkTimes ) {
        FaultMakeTask task = null;
        NodeRestart nr = new NodeRestart( node );
        task = new FaultMakeTask( nr, maxDelay, duration, checkTimes );
        return task;
    }

    /**
     * 
     * @param node
     * @param maxDelay
     *            最大延迟启动时间s
     * @param duration
     *            持续时间s
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( NodeWrapper node,
            int maxDelay, int duration ) {
        FaultMakeTask task = null;
        NodeRestart nr = new NodeRestart( node );
        task = new FaultMakeTask( nr, maxDelay, duration, 300 );
        return task;
    }
}
