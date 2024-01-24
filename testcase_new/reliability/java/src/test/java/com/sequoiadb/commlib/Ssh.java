package com.sequoiadb.commlib;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Logger;

import com.jcraft.jsch.Channel;
import com.jcraft.jsch.ChannelExec;
import com.jcraft.jsch.ChannelSftp;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @author huangqiaohui
 */
public class Ssh {
    private final static Logger log = Logger.getLogger( Ssh.class.getName() );

    private static final int CHANNEL_CONNECT_TIMEOUT = 600 * 1000;
    private static final int GETRESULT_TIMEOUT = 600 * 1000;
    private String host;
    private String username;
    private String password;
    private int port;
    private String stdout;
    private String stderr;
    private int exitStatus;
    private Session session = null;
    // ssh建立的后台命令集合（key：Channel id ，value：Channel）
    private Map< Integer, Channel > backgroundCMD = new HashMap< Integer, Channel >();

    /**
     * 使用给定参数及22端口创建ssh对象
     *
     * @param host
     * @param username
     * @param password
     * @throws ReliabilityException
     */
    public Ssh( String host, String username, String password )
            throws ReliabilityException {
        this( host, username, password, 22 );
    }

    /**
     * 使用给定参数创建ssh对象
     *
     * @param host
     * @param username
     * @param password
     * @param port
     * @throws ReliabilityException
     */
    public Ssh( String host, String username, String password, int port )
            throws ReliabilityException {
        super();
        this.host = host;
        this.username = username;
        this.password = password;
        this.port = port;
        JSch jsch = new JSch();
        TestLogger logger = new TestLogger();
        logger.setFileHandler();
        JSch.setLogger( logger );
        try {
            session = jsch.getSession( username, host, port );
            session.setPassword( password );
            session.setConfig( "StrictHostKeyChecking", "no" );
            session.connect( CHANNEL_CONNECT_TIMEOUT );

        } catch ( JSchException e ) {
            if ( session != null ) {
                session.disconnect();
            }
            e.printStackTrace();
            throw new FaultException( e );
        }
    }

    /**
     * 本地发送文件至远程主机
     *
     * @param localPath
     * @param remotePath
     * @throws ReliabilityException
     */
    public void scpTo( String localPath, String remotePath )
            throws ReliabilityException {
        ChannelSftp channel = null;
        try {
            channel = ( ChannelSftp ) session.openChannel( "sftp" );
            channel.connect( CHANNEL_CONNECT_TIMEOUT );
            channel.put( localPath, remotePath );
        } catch ( Exception e ) {
            e.printStackTrace();
            throw new FaultException( e );
        } finally {
            if ( channel != null ) {
                channel.disconnect();
            }
        }
    }

    /**
     * 下载远程主机文件至本地
     *
     * @param localPath
     * @param remotePath
     * @throws ReliabilityException
     */
    public void scpFrom( String localPath, String remotePath )
            throws ReliabilityException {
        ChannelSftp channel = null;
        try {
            channel = ( ChannelSftp ) session.openChannel( "sftp" );
            channel.connect( CHANNEL_CONNECT_TIMEOUT );
            channel.get( remotePath, localPath );
        } catch ( Exception e ) {
            e.printStackTrace();
            throw new FaultException( e );
        } finally {
            if ( channel != null ) {
                channel.disconnect();
            }
        }
    }

    /**
     * 在远程主机上执行命令，并等待其执行结果，标准输出存入stdout，标准出错存入stderr,返回值存入exitStatus(注意：
     * 每一次调用exec都将覆盖上一次的执行结果,返回值不为零将抛出异常)
     *
     * @param command
     * @return
     * @throws ReliabilityException
     */
    public void exec( String command ) throws ReliabilityException {
        Channel channel = null;
        try {
            channel = session.openChannel( "exec" );
            ( ( ChannelExec ) channel ).setCommand( command );
            channel.setInputStream( null );
            getResult( channel, GETRESULT_TIMEOUT );
            if ( exitStatus != 0 ) {
                throw new ReliabilityException(
                        "ssh failed to execute commond '" + command
                                + "',stderr:" + stderr + " ,stdout:" + stdout
                                + ",errcode: " + exitStatus );
            }
        } catch ( IOException | JSchException e ) {
            e.printStackTrace();
            throw new FaultException( e );
        } finally {
            if ( channel != null ) {
                channel.disconnect();
            }
        }
    }

    /**
     * 将命令发送至远程主机，返回当前命令的channelId，backgroundCMD将会记录执行本条命令的ChannelId及其Channel对象，
     * waitBackgroudCMDDown方法可以根据channelid检测命令的执行结果
     *
     * @param command
     * @return channelID
     * @throws JSchException
     */
    public int execBackground( String command ) throws ReliabilityException {
        Channel channel = null;
        try {
            channel = session.openChannel( "exec" );
            ( ( ChannelExec ) channel ).setCommand( command );
            channel.setInputStream( null );
            channel.connect( CHANNEL_CONNECT_TIMEOUT );
            backgroundCMD.put( channel.getId(), channel );
            return channel.getId();
        } catch ( JSchException e ) {
            if ( channel != null ) {
                channel.disconnect();
            }
            e.printStackTrace();
            throw new FaultException( e );
        }

    }

    /**
     * 等待给定channelId所执行的命令结束，覆盖stdout，stderr，exitstatus保存结果
     *
     * @param channelId
     * @return
     * @throws ReliabilityException
     */
    public void waitBackgroudCMDDown( int channelId )
            throws ReliabilityException {
        waitBackgroudCMDDown( channelId, GETRESULT_TIMEOUT );
    }

    /**
     * 等待给定channelId所执行的命令结束，覆盖stdout，stderr，exitstatus保存结果
     *
     * @param timeOutSecond
     * @param channelId
     * @return
     * @throws ReliabilityException
     */
    public void waitBackgroudCMDDown( int channelId, int timeOutSecond )
            throws ReliabilityException {
        Channel channel = backgroundCMD.get( channelId );
        if ( channel == null ) {
            throw new ReliabilityException(
                    "ssh can not find this channel id(can not check channel id twice)" );
        }
        backgroundCMD.remove( channelId );
        try {
            getResult( channel, timeOutSecond );
        } catch ( IOException e ) {
            throw new FaultException( e );
        } catch ( JSchException e ) {
            e.printStackTrace();
            throw new FaultException( e );
        } finally {
            channel.disconnect();
        }
    }

    /**
     * 关闭Session，关闭backgroundCMD中的Channel（但这些未结束的后台命令可能仍会在远程主机正常执行）
     */
    public void disconnect() {
        for ( Channel channel : backgroundCMD.values() ) {
            channel.disconnect();
        }
        if ( this.session != null ) {
            this.session.disconnect();
        }
    }

    public String getSdbInstallDir() throws ReliabilityException {
        Ssh ssh = new Ssh( host, username, password );
        String dir = null;
        try {
            ssh.exec( "cat /etc/default/sequoiadb |grep INSTALL_DIR" );
            String str = ssh.getStdout();
            if ( str.length() <= 0 ) {
                throw new ReliabilityException(
                        "exec command:cat /etc/default/sequoiadb |grep INSTALL_DIR can not find sequoiadb install dir" );
            }
            dir = str.substring( str.indexOf( "=" ) + 1, str.length() - 1 );
        } finally {
            ssh.disconnect();
        }
        return dir;

    }

    private void getResult( Channel channel, long timeOut )
            throws IOException, JSchException {
        StringBuffer stdoutBf = new StringBuffer();
        StringBuffer stderrBf = new StringBuffer();
        InputStream er = ( ( ChannelExec ) channel ).getErrStream();
        InputStream in = channel.getInputStream();
        byte[] tmp = new byte[ 1024 ];
        long timer = System.currentTimeMillis();
        channel.connect( CHANNEL_CONNECT_TIMEOUT );
        while ( true ) {
            while ( in.available() > 0 ) {
                int i = in.read( tmp, 0, 1024 );
                if ( i < 0 ) {
                    break;
                }
                stdoutBf.append( new String( tmp, 0, i ) );

                if ( System.currentTimeMillis() - timer > timeOut ) {
                    break;
                }
            }
            while ( er.available() > 0 ) {
                int i = er.read( tmp, 0, 1024 );
                if ( i < 0 )
                    break;
                stderrBf.append( new String( tmp, 0, i ) );
                if ( System.currentTimeMillis() - timer > timeOut ) {
                    break;
                }
            }

            if ( channel.isClosed() ) {
                if ( in.available() > 0 || er.available() > 0 ) {
                    continue;
                }
                break;
            }

            if ( !channel.isClosed() && channel.getExitStatus() == 0 ) {
                break;
            }

            try {
                Thread.sleep( 200 );
            } catch ( Exception e ) {
                // ignore
            }

            if ( System.currentTimeMillis() - timer > timeOut ) {
                break;
            }
        }

        stdout = stdoutBf.toString();
        stderr = stderrBf.toString();
        exitStatus = channel.getExitStatus();
    }

    public String getStdout() {
        return stdout;
    }

    public String getStderr() {
        return stderr;
    }

    public int getExitStatus() {
        return exitStatus;
    }

    public String getHost() {
        return host;
    }

    public String getUsername() {
        return username;
    }

    public String getPassword() {
        return password;
    }

    public int getPort() {
        return port;
    }

    public Session getSession() {
        return session;
    }

    private static final AtomicInteger sync = new AtomicInteger( 0 );
    private static PrintStream logger = null;

    class TestLogger implements com.jcraft.jsch.Logger {
        public void setFileHandler() {
            if ( sync.compareAndSet( 0, 1 ) ) {
                try {
                    Date date = new Date();
                    SimpleDateFormat format = new SimpleDateFormat(
                            "yyyy-MM-dd_HH.mm.ss" );
                    String strTime = format.format( date.getTime() );
                    logger = new PrintStream(
                            SdbTestBase.workDir + "/jsch" + strTime + ".log" );

                } catch ( FileNotFoundException e ) {
                    e.printStackTrace();
                }
            }
        }

        @Override
        public boolean isEnabled( int level ) {
            return true;
        }

        @Override
        public void log( int level, String msg ) {
            Date date = new Date();
            SimpleDateFormat format = new SimpleDateFormat(
                    "yyyy-MM-dd_HH.mm.ss" );
            if ( level == 0 ) {
                logger.append( "debug:" );
            } else if ( level == 1 ) {
                logger.append( "info:" );
            } else if ( level == 2 ) {
                logger.append( "warn:" );
            } else if ( level == 3 ) {
                logger.append( "error:" );
            } else {
                logger.append( "fatal:" );
            }
            logger.append( format.format( date.getTime() ) );
            logger.append( " " + msg );
            logger.append( "\n" );
            logger.flush();
        }
    }
}
