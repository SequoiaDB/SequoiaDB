package com.sequoiadb.lob.randomwrite;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.types.ObjectId;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by laojingtang on 17-11-20.
 */
class DbLobReadTask extends DbClOperateTask {
    private byte[] result;
    private int begin, length;
    private ObjectId id;
    private int retryTimes = 1;

    public void setRetryTimes( int retryTimes ) {
        this.retryTimes = retryTimes;
    }

    public int getLength() {
        return length;
    }

    public int getBegin() {
        return begin;
    }

    public DbLobReadTask( String csName, String clName, int begin, int length,
            ObjectId id ) {
        super( csName, clName );
        this.begin = begin;
        this.length = length;
        this.id = id;
        this.retryTimes = 5;
    }

    public DbLobReadTask( Sequoiadb db, String csName, String clName, int begin,
            int length, ObjectId id ) {
        super( db, csName, clName );
        this.begin = begin;
        this.length = length;
        this.id = id;
    }

    public byte[] getResult() {
        return result;
    }

    @Override
    protected void exec() throws Exception {
        for ( int i = 0; i < retryTimes; i++ ) {
            DBLob lob = null;
            try {
                lob = this.dbcl.openLob( id );
                result = new byte[ length ];
                lob.seek( begin, DBLob.SDB_LOB_SEEK_SET );
                lob.read( result );
                lob.close();
                break;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() == SDBError.SDB_FNE.getErrorCode() ) {
                    if ( i == retryTimes )
                        throw e;
                    Thread.sleep( 500 );
                    continue;
                } else
                    throw e;
            } finally {
                if ( lob != null )
                    lob.close();
            }
        }
    }
}

class DbLobWriteTask extends DbClOperateTask {

    final byte[] data;
    final ObjectId id;
    final int begin;

    public DbLobWriteTask( Sequoiadb db, String csName, String clName,
            byte[] data, ObjectId id, int begin ) {
        super( db, csName, clName );
        this.data = data;
        this.id = id;
        this.begin = begin;
    }

    public DbLobWriteTask( String csName, String clName, ObjectId id, int begin,
            byte[] data ) {
        super( csName, clName );
        this.id = id;
        this.begin = begin;
        this.data = data;
    }

    @Override
    protected void exec() throws Exception {
        DBLob lob = this.dbcl.openLob( id, DBLob.SDB_LOB_WRITE );
        lob.lock( begin, data.length );
        lob.seek( begin, DBLob.SDB_LOB_SEEK_SET );
        lob.write( data );
        lob.close();
    }
}

/**
 * Created by laojingtang on 17-11-6.
 */
abstract class DbClOperateTask extends Thread {
    Exception _exception = null;
    protected Sequoiadb db = null;
    protected DBCollection dbcl = null;
    protected List< Integer > ignoreErrCodes = new ArrayList<>( 10 );

    private Sequoiadb getDefaultSdb() {
        String coordUrl = SdbTestBase.getDefaultCoordUrl();
        return new Sequoiadb( coordUrl, "", "" );
    }

    public DbClOperateTask( String csName, String clName ) {
        db = getDefaultSdb();
        dbcl = db.getCollectionSpace( csName ).getCollection( clName );
    }

    public DbClOperateTask( Sequoiadb db, String csName, String clName ) {
        this.db = db;
        this.dbcl = db.getCollectionSpace( csName ).getCollection( clName );
    }

    public DbClOperateTask ignoreExceptionCode( int errCode ) {
        ignoreErrCodes.add( errCode );
        return this;
    }

    public int getSdbErrCode() {
        if ( _exception == null )
            return 0;
        if ( _exception instanceof BaseException )
            return ( ( BaseException ) _exception ).getErrorCode();
        else
            return 0;
    }

    public boolean isTaskSuccess() {
        return this._exception == null ? true : false;
    }

    public Exception getException() {
        return _exception;
    }

    protected abstract void exec() throws Exception;

    public void run() {
        try {
            exec();
        } catch ( Exception e ) {
            if ( e instanceof BaseException ) {
                int c = ( ( BaseException ) e ).getErrorCode();
                if ( !ignoreErrCodes.contains( c ) )
                    this._exception = e;
            } else {
                this._exception = e;
            }
        } finally {
            this.db.close();
        }
    }

    public String getErrorMsg() {
        if ( _exception == null )
            return "";
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        PrintStream printStream = new PrintStream( bytes );
        printStream.println();
        printStream
                .println( "------SDB Task: " + getName() + " err msg start: " );
        _exception.printStackTrace( printStream );
        printStream.println( "------SDB Task: " + getName() + " err msg end." );
        printStream.flush();
        return bytes.toString();
    }
}