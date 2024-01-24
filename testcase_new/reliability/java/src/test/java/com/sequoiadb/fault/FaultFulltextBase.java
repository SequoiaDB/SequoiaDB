package com.sequoiadb.fault;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;

import org.bson.BSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;

public class FaultFulltextBase extends Fault {

    protected String user;
    protected String password;
    protected Ssh ssh;
    protected String hostName;
    protected String svcName;
    protected int port;
    protected String localPath;
    protected String remotePath;
    protected String makeScriptName = "faultFulltext.sh";
    protected String restoreScriptName = "restoreFulltext.sh";
    protected String progName;
    protected String killRestart;
    protected String pid = "-1";
    protected String cmdDir;
    protected String cmdArgs;
    protected final static Logger log = Logger
            .getLogger( FaultFulltextBase.class.getName() );

    public FaultFulltextBase( String name ) {
        super( name );
    }

    @Override
    public void make() throws FaultException {
        if ( svcName != null ) {
            log.info( "Make " + this.getName() + " " + hostName + ": "
                    + svcName );
        } else {
            svcName = "";
            log.info( "Make " + this.getName() + " " + hostName );
        }

        try {
            ssh.exec( remotePath + "/" + makeScriptName + " " + progName + " "
                    + killRestart + " " + svcName );
            getMakeStdout();
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    /**
     * 继承FaultFulltextBase需要实现该方法，用来解析make()的返回值，返回值pid:cmdDir
     * 
     * @exem String stdout = ssh.getStdout().trim(); pid = stdout.split(":")[0];
     *       cmdDir = stdout.split(":")[1];
     */
    protected void getMakeStdout() {
    }

    protected String beforeCheckMakeResult() {
        return null;
    }

    @Override
    public boolean checkMakeResult() throws FaultException {
        if ( pid.equals( "-1" ) ) {
            return false;
        }
        try {
            String sshStr = beforeCheckMakeResult();
            if ( sshStr == null ) {
                sshStr = "ps -ef | grep " + progName
                        + " | grep -v grep | grep '" + svcName
                        + "' | awk '{print $2}'";
            }
            ssh.exec( sshStr );
            if ( ssh.getStdout().trim().length() <= 0 ) {
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
        if ( svcName != "" ) {
            log.info( "Restore " + this.getName() + " " + hostName + ": "
                    + svcName );
        } else {
            log.info( "Restore " + this.getName() + " " + hostName );
        }

        try {
            beforeRestore();
            ssh.exec( remotePath + "/" + restoreScriptName + " " + cmdArgs );
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    /**
     * 继承FaultFulltextBase需要实现该方法，在restore()之前给cmdArgs赋值，可以调用setRestoreArgs()方法，
     * 至少有一个参数且必须是进程名
     * 
     * @exem cmdArgs = setRestoreArgs(progName, cmdDir, sdbseadapterDir,
     *       svcName);
     */
    protected void beforeRestore() {
    }

    protected String setRestoreArgs( String... args ) {
        String ret = "";
        for ( String arg : args ) {
            ret += " " + arg;
        }
        return ret;
    }

    protected String beforeCheckRestoreResult() {
        return null;
    }

    @Override
    public boolean checkRestoreResult() throws FaultException {
        try {
            String sshStr = beforeCheckRestoreResult();
            if ( sshStr == null ) {
                sshStr = "ps -ef | grep " + progName
                        + " | grep -v grep | grep '" + svcName
                        + "' | awk '{print $2}'";
            }
            int checkFlag = 0;
            while ( checkFlag++ < 120 ) {
                ssh.exec( sshStr );
                if ( ssh.getStdout().trim().length() > 0 ) {
                    break;
                }
                try {
                    Thread.sleep( 200 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }
            if ( ssh.getStdout().trim().length() <= 0 ) {
                return false;
            } else {
                queryAndCheck();
                return true;
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void init() throws FaultException {
        try {
            ssh = new Ssh( hostName, user, password, port );
            ssh.scpTo( localPath + "/" + makeScriptName, remotePath + "/" );
            ssh.scpTo( localPath + "/" + restoreScriptName, remotePath + "/" );
            ssh.exec( "chmod 777 " + remotePath + "/" + makeScriptName );
            ssh.exec( "chmod 777 " + remotePath + "/" + restoreScriptName );
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    @Override
    public void fini() throws FaultException {
        try {
            if ( ssh != null ) {
                ssh.exec( "rm -rf " + remotePath + "/" + makeScriptName );
                ssh.exec( "rm -rf " + remotePath + "/" + restoreScriptName );
            }
        } catch ( ReliabilityException e ) {
            throw new FaultException( e );
        }
    }

    protected void queryAndCheck() throws FaultException {
        Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        String csName = "reliabilityFaultFulltextCSName";
        String clName = "reliabilityFaultFulltextCSName";
        String idxName = "reliabilityFaultFulltextIdxName";
        try {
            DBCollection cl = db1.createCollectionSpace( csName )
                    .createCollection( clName, ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'_id':1}, ShardingType:'hash', AutoSplit:true}" ) );
            cl.createIndex( idxName, "{'a':'text'}", false, false );
            List< BSONObject > list = new ArrayList<>();
            for ( int i = 0; i < 1000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{a:'a" + i + "'}" );
                list.add( obj );
            }
            cl.insert( list );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            while ( true ) {
                try {
                    DBCursor cursor1 = cl.query( "", "", "{_id:1}", "" );
                    DBCursor cursor2 = cl2.query(
                            "{'':{'$Text':{'query':{'match_all':{}}}}}", "",
                            "{_id:1}", "" );
                    if ( FullTextUtils.isCLRecordsConsistency( cursor1,
                            cursor2 ) ) {
                        break;
                    }
                } catch ( BaseException e ) {
                    // SEQUOIADBMAINSTREAM-4813 暂时规避 -6
                    // SEQUOIADBMAINSTREAM-4811 暂时规避 -10
                    if ( -79 != e.getErrorCode() && -52 != e.getErrorCode()
                            && -13 != e.getErrorCode() && -6 != e.getErrorCode()
                            && -10 != e.getErrorCode() ) {
                        throw e;
                    }
                }
            }
        } catch ( BaseException e ) {
            throw new FaultException( e );
        } finally {
            FullTextDBUtils.dropCollectionSpace( db1, csName );
            db1.close();
            db2.close();
        }
    }
}
