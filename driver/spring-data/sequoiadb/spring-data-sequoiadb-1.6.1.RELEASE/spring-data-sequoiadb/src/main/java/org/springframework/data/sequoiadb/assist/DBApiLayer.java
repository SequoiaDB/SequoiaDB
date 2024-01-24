package org.springframework.data.sequoiadb.assist;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.exception.SDBError;

/**
 * Concrete extension of abstract {@code DB} class.
 *
 * @deprecated This class is NOT part of the public API. It will be dropped in 3.x releases.
 */
@Deprecated
@SuppressWarnings("deprecation")
public class DBApiLayer extends DB {

    private final Sdb _sdb;
    private final ConnectionManager _cm;
    private final String _csName;

    /**
     * @param sdb the Sdb instance
     * @param name the database name
     * @param connector the connector.  This must be an instance of DBTCPConnector.
     */
    protected DBApiLayer(Sdb sdb, String name , DBConnector connector ){
        super(sdb, name );
        _sdb = sdb;
        _csName = name;
        _cm = _sdb.getConnectionManager();
    }

    public void requestStart(){
        throw new UnsupportedOperationException("not supported!");
    }

    public void requestDone(){
        throw new UnsupportedOperationException("not supported!");
    }

    public void requestEnsureConnection(){
        throw new UnsupportedOperationException("not supported!");
    }

    public WriteResult addUser( String username , char[] passwd, boolean readOnly ){
        throw new UnsupportedOperationException("not supported!");
    }

    public WriteResult removeUser( String username ){
        throw new UnsupportedOperationException("not supported!");
    }

    protected DBCollectionImpl doGetCollection( String name ){
        final String myName = name;
        return _cm.execute(_csName, new CSCallback<DBCollectionImpl>() {
            @Override
            public DBCollectionImpl doInCS(CollectionSpace cs) throws com.sequoiadb.exception.BaseException {
                if ( !cs.isCollectionExist(myName) ) {
                    throw new com.sequoiadb.exception.BaseException(SDBError.SDB_DMS_NOTEXIST,
                            String.format("Collection [%s.%s] does not exist in database", _csName, myName));
                }
                return new DBCollectionImpl(new DBApiLayer(_sdb, _csName, null), myName);
            }
        });
    }

    /**
     * @param force true if should clean regardless of number of dead cursors
     * @throws BaseException
     */
    public void cleanCursors( boolean force ){
    }

}

