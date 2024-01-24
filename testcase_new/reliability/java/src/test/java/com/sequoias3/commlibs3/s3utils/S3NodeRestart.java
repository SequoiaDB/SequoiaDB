package com.sequoias3.commlibs3.s3utils;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.fault.Fault;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoias3.commlibs3.s3utils.bean.S3NodeWrapper;

/**
 * Created by fanyu on 2019/6/6.
 */
public class S3NodeRestart extends Fault {

    private S3NodeWrapper node;

    public S3NodeRestart( S3NodeWrapper node ) {
        super( "nodeRestart" );
        this.node = node;
    }

    @Override
    public void make() throws FaultException {
        System.out.println( "target node:" + this.node.toString() );
        try {
            this.node.stop();
        } catch ( Exception e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public boolean checkMakeResult() throws FaultException {
        try {
            return !this.node.isNodeActive();
        } catch ( Exception e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void restore() throws FaultException {
        try {
            this.node.start();
        } catch ( Exception e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public boolean checkRestoreResult() throws FaultException {
        try {
            return this.node.isNodeActive();
        } catch ( Exception e ) {
            throw new FaultException( e );
        }
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
    public static FaultMakeTask getFaultMakeTask( S3NodeWrapper node,
            int maxDelay, int duration, int checkTimes ) {
        S3NodeRestart nr = new S3NodeRestart( node );
        return new FaultMakeTask( nr, maxDelay, duration, checkTimes );
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
    public static FaultMakeTask getFaultMakeTask( S3NodeWrapper node,
            int maxDelay, int duration ) {
        return new FaultMakeTask( new S3NodeRestart( node ), maxDelay, duration,
                10 );
    }
}
