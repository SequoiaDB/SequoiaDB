package com.sequoiadb.metaopr.commons;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ServerAddress;
import org.bson.BSONObject;

import java.io.Closeable;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Map;

/**
 * @FileName
 * @Ajjuthor laojingtang
 * @Date 17-5-4
 * @Version 1.00
 */
class MySequoiadb implements Closeable {
    private Sequoiadb _db;
    private SequoiadbDatasource _ds;

    public Sequoiadb getSequoiadb() {
        return _db;
    }

    public MySequoiadb( Sequoiadb db, SequoiadbDatasource ds ) {
        _db = db;
        _ds = ds;
    }

    public static void initClient( ClientOptions options ) {
        Sequoiadb.initClient( options );
    }

    @Deprecated
    public ServerAddress getServerAddress() {
        return _db.getServerAddress();
    }

    public String getHost() {
        return _db.getHost();
    }

    public int getPort() {
        return _db.getPort();
    }

    @Deprecated
    public boolean isEndianConvert() {
        return _db.isEndianConvert();
    }

    public ByteOrder getByteOrder() {
        return _db.getByteOrder();
    }

    public long getLastUseTime() {
        return _db.getLastUseTime();
    }

    public void createUser( String username, String password )
            throws BaseException {
        _db.createUser( username, password );
    }

    public void removeUser( String username, String password )
            throws BaseException {
        _db.removeUser( username, password );
    }

    @Deprecated
    public void disconnect() throws BaseException {
        _db.disconnect();
    }

    public void releaseResource() {
        _db.releaseResource();
    }

    public boolean isClosed() {
        return _db.isClosed();
    }

    public boolean isValid() throws BaseException {
        return _db.isValid();
    }

    @Deprecated
    public void changeConnectionOptions( ConfigOptions options )
            throws BaseException {
        _db.changeConnectionOptions( options );
    }

    public CollectionSpace createCollectionSpace( String csName )
            throws BaseException {
        return _db.createCollectionSpace( csName );
    }

    public CollectionSpace createCollectionSpace( String csName, int pageSize )
            throws BaseException {
        return _db.createCollectionSpace( csName, pageSize );
    }

    public CollectionSpace createCollectionSpace( String csName,
            BSONObject options ) throws BaseException {
        return _db.createCollectionSpace( csName, options );
    }

    public void dropCollectionSpace( String csName ) throws BaseException {
        _db.dropCollectionSpace( csName );
    }

    public void loadCollectionSpace( String csName, BSONObject options )
            throws BaseException {
        _db.loadCollectionSpace( csName, options );
    }

    public void unloadCollectionSpace( String csName, BSONObject options )
            throws BaseException {
        _db.unloadCollectionSpace( csName, options );
    }

    public void renameCollectionSpace( String oldName, String newName )
            throws BaseException {
        _db.renameCollectionSpace( oldName, newName );
    }

    public void sync( BSONObject options ) throws BaseException {
        _db.sync( options );
    }

    public void sync() throws BaseException {
        _db.sync();
    }

    public CollectionSpace getCollectionSpace( String csName )
            throws BaseException {
        return _db.getCollectionSpace( csName );
    }

    public boolean isCollectionSpaceExist( String csName )
            throws BaseException {
        return _db.isCollectionSpaceExist( csName );
    }

    public DBCursor listCollectionSpaces() throws BaseException {
        return _db.listCollectionSpaces();
    }

    public ArrayList< String > getCollectionSpaceNames() throws BaseException {
        return _db.getCollectionSpaceNames();
    }

    public DBCursor listCollections() throws BaseException {
        return _db.listCollections();
    }

    public ArrayList< String > getCollectionNames() throws BaseException {
        return _db.getCollectionNames();
    }

    public ArrayList< String > getStorageUnits() throws BaseException {
        return _db.getStorageUnits();
    }

    public void resetSnapshot() throws BaseException {
        _db.resetSnapshot();
    }

    public DBCursor getList( int listType, BSONObject query,
            BSONObject selector, BSONObject orderBy ) throws BaseException {
        return _db.getList( listType, query, selector, orderBy );
    }

    public void flushConfigure( BSONObject options ) throws BaseException {
        _db.flushConfigure( options );
    }

    public void execUpdate( String sql ) throws BaseException {
        _db.execUpdate( sql );
    }

    public DBCursor exec( String sql ) throws BaseException {
        return _db.exec( sql );
    }

    public DBCursor getSnapshot( int snapType, String matcher, String selector,
            String orderBy ) throws BaseException {
        return _db.getSnapshot( snapType, matcher, selector, orderBy );
    }

    public DBCursor getSnapshot( int snapType, BSONObject matcher,
            BSONObject selector, BSONObject orderBy ) throws BaseException {
        return _db.getSnapshot( snapType, matcher, selector, orderBy );
    }

    public void beginTransaction() throws BaseException {
        _db.beginTransaction();
    }

    public void commit() throws BaseException {
        _db.commit();
    }

    public void rollback() throws BaseException {
        _db.rollback();
    }

    public void crtJSProcedure( String code ) throws BaseException {
        _db.crtJSProcedure( code );
    }

    public void rmProcedure( String name ) throws BaseException {
        _db.rmProcedure( name );
    }

    public DBCursor listProcedures( BSONObject condition )
            throws BaseException {
        return _db.listProcedures( condition );
    }

    public Sequoiadb.SptEvalResult evalJS( String code ) throws BaseException {
        return _db.evalJS( code );
    }

    public void backupOffline( BSONObject options ) throws BaseException {
        _db.backupOffline( options );
    }

    public DBCursor listBackup( BSONObject options, BSONObject matcher,
            BSONObject selector, BSONObject orderBy ) throws BaseException {
        return _db.listBackup( options, matcher, selector, orderBy );
    }

    public void removeBackup( BSONObject options ) throws BaseException {
        _db.removeBackup( options );
    }

    public DBCursor listTasks( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint ) throws BaseException {
        return _db.listTasks( matcher, selector, orderBy, hint );
    }

    public void waitTasks( long[] taskIDs ) throws BaseException {
        _db.waitTasks( taskIDs );
    }

    public void cancelTask( long taskID, boolean isAsync )
            throws BaseException {
        _db.cancelTask( taskID, isAsync );
    }

    public void setSessionAttr( BSONObject options ) throws BaseException {
        _db.setSessionAttr( options );
    }

    public void closeAllCursors() throws BaseException {
        _db.closeAllCursors();
    }

    public DBCursor listReplicaGroups() throws BaseException {
        return _db.listReplicaGroups();
    }

    public boolean isDomainExist( String domainName ) throws BaseException {
        return _db.isDomainExist( domainName );
    }

    public Domain createDomain( String domainName, BSONObject options )
            throws BaseException {
        return _db.createDomain( domainName, options );
    }

    public void dropDomain( String domainName ) throws BaseException {
        _db.dropDomain( domainName );
    }

    public Domain getDomain( String domainName ) throws BaseException {
        return _db.getDomain( domainName );
    }

    public DBCursor listDomains( BSONObject matcher, BSONObject selector,
            BSONObject orderBy, BSONObject hint ) throws BaseException {
        return _db.listDomains( matcher, selector, orderBy, hint );
    }

    public ArrayList< String > getReplicaGroupNames() throws BaseException {
        return _db.getReplicaGroupNames();
    }

    public ArrayList< String > getReplicaGroupsInfo() throws BaseException {
        return _db.getReplicaGroupsInfo();
    }

    public ReplicaGroup getReplicaGroup( String rgName ) throws BaseException {
        return _db.getReplicaGroup( rgName );
    }

    public ReplicaGroup getReplicaGroup( int rgId ) throws BaseException {
        return _db.getReplicaGroup( rgId );
    }

    public ReplicaGroup createReplicaGroup( String rgName )
            throws BaseException {
        return _db.createReplicaGroup( rgName );
    }

    public void removeReplicaGroup( String rgName ) throws BaseException {
        _db.removeReplicaGroup( rgName );
    }

    public void activateReplicaGroup( String rgName ) throws BaseException {
        _db.activateReplicaGroup( rgName );
    }

    public void createReplicaCataGroup( String hostName, int port,
            String dbPath, Map< String, String > configure ) {
        _db.createReplicaCataGroup( hostName, port, dbPath, configure );
    }

    public void close() throws BaseException {
        _ds.releaseConnection( _db );
    }
}
