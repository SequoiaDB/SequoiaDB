package org.springframework.data.mongodb.assist;

import com.sequoiadb.base.*;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;

/**
 * Created by tanzhaobo on 2017/9/9.
 */

interface DBCallback<T> {
    T doInDB(Sequoiadb db) throws BaseException;
}

interface CSCallback<T> {
    T doInCS(CollectionSpace cs) throws BaseException;
}

interface CLCallback<T> {
    T doInCL(com.sequoiadb.base.DBCollection cl) throws BaseException;
}

interface IQueryResult {
    com.sequoiadb.base.DBCursor getCursor();
    com.sequoiadb.base.Sequoiadb getSequoiadb();
}

interface QueryInCLCallback<T extends IQueryResult> {
    T doQuery(com.sequoiadb.base.DBCollection cl) throws BaseException;
}

interface QueryInSessionCallback<T extends IQueryResult> {
    T doQuery(com.sequoiadb.base.Sequoiadb db) throws BaseException;
}

class DBQueryResult implements IQueryResult {

    private com.sequoiadb.base.DBCursor _cursor;
    private com.sequoiadb.base.Sequoiadb _sequoiadb;

    DBQueryResult(com.sequoiadb.base.DBCursor cursor, com.sequoiadb.base.Sequoiadb sequoiadb) {
        this._cursor = cursor;
        this._sequoiadb = sequoiadb;
    }

    @Override
    public DBCursor getCursor() {
        return _cursor;
    }

    @Override
    public Sequoiadb getSequoiadb() {
        return _sequoiadb;
    }
}
