package com.sequoias3.dao;

import com.sequoias3.exception.S3ServerException;

public interface DaoMgr {
    ConnectionDao getConnectionDao() throws S3ServerException;

    void releaseConnectionDao(ConnectionDao connection) throws S3ServerException;
}
