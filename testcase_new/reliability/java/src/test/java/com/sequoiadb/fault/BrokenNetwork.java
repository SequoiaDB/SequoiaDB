/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:BrokenNetwork.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.fault;

import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.FaultMakeTask;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;

public class BrokenNetwork extends Fault {
    private final static Logger log = Logger
            .getLogger( BrokenNetwork.class.getName() );

    private String hostName;
    private String user;
    private String passwd;
    private int duration;
    private int port = 22;
    private String remotePath;
    private Ssh ssh;
    private long brokenTime;
    private GroupWrapper group = null;;
    private final String localScriptPath = SdbTestBase.scriptDir;
    private final String scriptName = "brokenNetwork.sh";

    /**
     * 
     * @param hostName
     * @param duration
     */
    public BrokenNetwork( String hostName, int duration ) {
        super( "brokenNetwork" );
        this.hostName = hostName;
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.duration = duration;
        this.remotePath = SdbTestBase.workDir;
        this.port = 22;
    }

    public BrokenNetwork( GroupWrapper group, int times ) {
        super( "brokenNetwork" );
        this.user = "root";
        this.passwd = SdbTestBase.rootPwd;
        this.remotePath = SdbTestBase.workDir;
        this.port = 22;
        this.group = group;
        this.duration = times;

    }

    public void make() throws FaultException {
        try {
            if ( group == null ) {
                log.info( "brokenHost: " + hostName );
                ssh.execBackground( "nohup " + remotePath + "/" + scriptName
                        + " " + duration + " > /tmp/brokenNet.log &" );
                brokenTime = System.currentTimeMillis();
            } else {
                continuouslyMakeOnPri();
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    public void continuouslyMakeOnPri() throws ReliabilityException {
        Set< String > allHosts = group.getAllHosts();
        System.out.println( allHosts );
        List< String > brokenHost = new ArrayList< String >();
        String host = group.getMaster().hostName();
        System.out.println( "1:" + host );
        System.out.println( "Begin ContinuouslyMakeBrokenNetOnPri:"
                + group.getGroupName() );
        for ( int i = 0; i < duration; i++ ) {
            ssh = new Ssh( host, user, passwd, port );
            ssh.scpTo( localScriptPath + "/" + scriptName, remotePath + "/" );
            ssh.exec( "chmod 777 " + remotePath + "/" + scriptName );
            ssh.execBackground( "nohup " + remotePath + "/" + scriptName
                    + " 15 > /tmp/brokenNet.log &" );
            ssh.disconnect();

            while ( true ) {
                if ( ping( host ) ) {
                    break;
                }
            }

            brokenHost.add( host );
            System.out.println(
                    "Broken network:" + host + ",Waitting group reelect...." );
            allHosts.removeAll( brokenHost );
            if ( allHosts.size() <= 0 ) {
                allHosts.addAll( brokenHost );
                brokenHost.clear();
            }
            boolean breakFlag = false;
            Exception exception1 = new Exception();
            Exception exception2 = new Exception();
            while ( true ) {
                try {
                    for ( NodeWrapper node : group.getNodes() ) {
                        // System.out.println(node.hostName());
                        try {
                            if ( node.isMaster( false ) ) {
                                host = node.hostName();
                                breakFlag = true;
                                break;
                            }
                        } catch ( Exception e ) {
                            if ( !e.getMessage()
                                    .equals( exception1.getMessage() ) ) {
                                System.out.println( "ignore some exceptions:"
                                        + e.getMessage() );
                                exception1 = e;
                            }
                            continue;
                        }
                    }
                    if ( breakFlag ) {
                        break;
                    }
                } catch ( Exception e ) {
                    if ( !e.getMessage().equals( exception2.getMessage() ) ) {
                        System.out.println(
                                "ignore some exceptions:" + e.getMessage() );
                        exception2 = e;
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e1 ) {
                        // ignore
                    }
                    continue;
                }
            }
            System.out.println( "reelect success,the host:" + host );
        }
    }

    public boolean checkMakeResult() throws FaultException {
        if ( group != null ) {
            return true;
        }
        int checkTime = 3;
        for ( int i = 0; i < checkTime; i++ ) {
            if ( ping() == false ) {
                return true;
            }
        }
        return false;
    }

    public void restore() throws FaultException {
        if ( group != null ) {
            return;
        }
        long diff = System.currentTimeMillis() - brokenTime;
        if ( diff < duration * 1000 ) {
            try {
                Thread.sleep( duration * 1000 - diff );
            } catch ( InterruptedException e ) {

            }
        }

    }

    public boolean checkRestoreResult() throws FaultException {
        int checkTime = 3;
        if ( group != null ) {
            Set< String > hosts = group.getAllHosts();
            for ( String host : hosts ) {
                for ( int i = 0; i < checkTime; i++ ) {
                    if ( ping( host ) ) {
                        return true;
                    }
                }
            }
            return true;
        }
        for ( int i = 0; i < checkTime; i++ ) {
            if ( ping() ) {
                return true;
            }
        }
        return false;
    }

    public boolean ping() throws FaultException {
        return ping( hostName );
    }

    public boolean ping( String host ) throws FaultException {
        String os = System.getProperties().getProperty( "os.name" );
        String cmd;
        if ( os.startsWith( "win" ) || os.startsWith( "Win" ) ) {
            cmd = "ping " + host + " -n 2 -w 2";
        } else {
            cmd = "ping " + host + " -c 2 -w 2";
        }

        Process pr = null;
        Runtime rt = Runtime.getRuntime();

        BufferedReader brIn = null;
        BufferedReader byErr = null;

        try {
            pr = rt.exec( cmd );
            InputStream stderr = pr.getErrorStream();
            InputStream stdin = pr.getInputStream();
            InputStreamReader isIn = new InputStreamReader( stdin );
            InputStreamReader isErr = new InputStreamReader( stderr );
            brIn = new BufferedReader( isIn );
            byErr = new BufferedReader( isErr );

            @SuppressWarnings("unused")
            String line = null;
            while ( ( line = brIn.readLine() ) != null )
                ;
            while ( ( line = byErr.readLine() ) != null )
                ;
            int exitcode = pr.waitFor();
            System.out.println( "exitcode: " + exitcode );
            if ( exitcode == 0 ) {
                return true;
            } else {
                return false;
            }
        } catch ( InterruptedException | IOException e ) {
            throw new FaultException( e );
        } finally {
            if ( brIn != null ) {
                try {
                    brIn.close();
                } catch ( IOException e ) {
                    e.printStackTrace();
                }
            }
            if ( byErr != null ) {
                try {
                    byErr.close();
                } catch ( IOException e ) {
                    e.printStackTrace();
                }
            }
            if ( pr != null ) {
                pr.destroy();
            }
        }
    }

    @Override
    public void init() throws FaultException {
        if ( group != null ) {
            return;
        }
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
        if ( group != null ) {
            return;
        }
        try {
            if ( ssh != null ) {
                ssh.disconnect();
            }
            if ( checkRestoreResult() ) {
                ssh = new Ssh( hostName, user, passwd );
                ssh.exec( "rm -rf " + remotePath + "/" + scriptName );
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        } finally {
            ssh.disconnect();
        }

    }

    /**
     * 对主机单次断网
     * 
     * @param hostName
     * @param maxDelay
     *            最大延迟启动时间
     * @param duration
     *            持续时间
     * @param checkTimes
     *            检查构造故障成功与否的次数（15）
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( String hostName, int maxDelay,
            int duration, int checkTimes ) {
        FaultMakeTask task = null;
        BrokenNetwork bn = new BrokenNetwork( hostName, duration );
        task = new FaultMakeTask( bn, maxDelay, duration, checkTimes );
        return task;
    }

    /**
     * 对主机单次断网
     * 
     * @param hostName
     * @param maxDelay
     *            最大延迟启动时间
     * @param duration
     *            持续时间
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( String hostName, int maxDelay,
            int duration ) {
        FaultMakeTask task = null;
        BrokenNetwork bn = new BrokenNetwork( hostName, duration );
        task = new FaultMakeTask( bn, maxDelay, duration, 50 );
        return task;
    }

    /**
     * 对group的主节点做连续断网操作
     * 
     * @param group
     * @param times
     *            断网次数（3-4）耗时大概1分钟
     * @param maxDelay
     *            延迟启动时间
     * @return
     */
    public static FaultMakeTask getFaultMakeTask( GroupWrapper group, int times,
            int maxDelay ) {
        FaultMakeTask task = null;
        BrokenNetwork bn = new BrokenNetwork( group, times );
        task = new FaultMakeTask( bn, maxDelay, 0, 100 );
        return task;
    }

}
