package org.springframework.data.mongodb.assist;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.net.UnknownHostException;
import java.util.NoSuchElementException;

class QueryResultIterator implements Cursor {

    private boolean _closed;
    private final ConnectionManager _cm;
    private final com.sequoiadb.base.DBCursor _cursor;
    private final com.sequoiadb.base.Sequoiadb _sequoiadb;
    private ServerAddress _host;

    QueryResultIterator(ConnectionManager cm, IQueryResult queryResult) {
        _closed = false;
        _cm = cm;
        _cursor = queryResult.getCursor();
        _sequoiadb = queryResult.getSequoiadb();
        try {
            _host = new ServerAddress(_sequoiadb.getServerAddress().getHost(), _sequoiadb.getServerAddress().getPort());
        } catch (UnknownHostException e) {
            _host = null;
        }
    }

    protected void finalize() {
        try {
            close();
        } catch (Exception e){
        }
    }

    public DBObject next() {
        if (_closed) {
            throw new IllegalStateException("Iterator has been closed");
        }

        if (!hasNext()) {
            throw new NoSuchElementException();
        }
        BSONObject record = _cursor.getNext();
        return Helper.BasicBSONObjectToBasicDBObject((BasicBSONObject) record);
    }

    DBObject current() {
        if (_closed) {
            throw new IllegalStateException("Iterator has been closed");
        }
        BasicBSONObject record = (BasicBSONObject)_cursor.getCurrent();
        return Helper.BasicBSONObjectToBasicDBObject(record);
    }

    public boolean hasNext() {
        if (_closed) {
            throw new IllegalStateException("Iterator has been closed");
        }

        return _cursor.hasNext();
    }

    public void remove(){
        throw new UnsupportedOperationException("can't remove a document via a query result iterator");
    }

    public void setBatchSize(int size){ return;}

    public long getCursorId(){
        return -1;
    }

    public void close(){
        if (!_closed) {
            _closed = true;
            try {
                if (_cursor != null) {
                    _cursor.close();
                }
            } finally {
                if (_cm != null && _sequoiadb != null) {
                    _cm.releaseConnection(_sequoiadb);
                }
            }
        }
    }

    public ServerAddress getServerAddress() {
        return _host;
    }

}

