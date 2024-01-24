package org.springframework.data.sequoiadb.assist;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;

import java.util.List;

/**
 * Created by tanzhaobo on 2017/9/6.
 */

class ConnectionManager {
    private boolean _hasClosed;
    private SequoiadbDatasource _ds;

    public ConnectionManager(List<String> urls, String username, String password, ConfigOptions nwOpt, DatasourceOptions dsOpt) {
        _hasClosed = false;
        _ds = new SequoiadbDatasource(urls, username, password, nwOpt, dsOpt);
    }

    public int getIdleConnCount() {
        return _ds.getIdleConnNum();
    }

    public int getUsedConnCount() {
        return _ds.getUsedConnNum();
    }

    public Sequoiadb getConnection() {
        if (_hasClosed) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED, "connection pool has closed");
        }
        try {
            return _ds.getConnection();
        } catch (InterruptedException e) {
            throw new BaseException(SDBError.SDB_INTERRUPT, "failed to get connection", e);
        }
    }

    public void releaseConnection(Sequoiadb db) {
        if (_hasClosed) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED, "connection pool has closed");
        }
        _ds.releaseConnection(db);
    }

    public void close() {
        if (_hasClosed) {
            return ;
        }
        _ds.close();
        _hasClosed = true;
    }

    public <T> T execute(DBCallback<T> action) {
        Sequoiadb db = null;
        try {
            db = this.getConnection();
            return action.doInDB(db);
        } finally {
            if (db != null) {
                this.releaseConnection(db);
            }
        }
    }

    public <T> T execute(String csName, CSCallback<T> action) {
        Sequoiadb db = null;
        try {
            db = this.getConnection();
            CollectionSpace cs = getOrCreateCollectionSpace(db, csName, null);
            return action.doInCS(cs);
        } finally {
            if (db != null) {
                this.releaseConnection(db);
            }
        }
    }

    public <T> T execute(String csName, String clName, CLCallback<T> action) {
        Sequoiadb db = null;
        try {
            db = this.getConnection();
            CollectionSpace cs = getOrCreateCollectionSpace(db, csName, null);
            DBCollection cl = getOrCreateCollection(cs, clName, null);
            return action.doInCL(cl);
        } finally {
            if (db != null) {
                this.releaseConnection(db);
            }
        }
    }

    public <T extends IQueryResult> T execute(String csName, String clName, QueryInCLCallback<T> action) {
        Sequoiadb db = null;
        try {
            // note: db is released in cursor.close().
            db = this.getConnection();
            CollectionSpace cs = db.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);
            return action.doQuery(cl);
        } catch(BaseException e) {
            if (db != null) {
                this.releaseConnection(db);
            }
            throw e;
        }
    }

    public <T extends IQueryResult> T execute(QueryInSessionCallback<T> action) {
        Sequoiadb db = null;
        try {
            // note: db is released in cursor.close().
            db = this.getConnection();
            return action.doQuery(db);
        } catch(BaseException e) {
            if (db != null) {
                this.releaseConnection(db);
            }
            throw e;
        }
    }


    private CollectionSpace getOrCreateCollectionSpace(Sequoiadb db, String csName, BSONObject options) {
        CollectionSpace cs;
        if (db.isCollectionSpaceExist(csName)) {
            cs = db.getCollectionSpace(csName);
        } else {
            cs = db.createCollectionSpace(csName, options);
        }
        return cs;
    }

    private DBCollection getOrCreateCollection(CollectionSpace cs, String clName, BSONObject options) {
        DBCollection cl;
        if (cs.isCollectionExist(clName)) {
            cl = cs.getCollection(clName);
        } else {
            cl = cs.createCollection(clName, options);
        }
        return cl;
    }

}
