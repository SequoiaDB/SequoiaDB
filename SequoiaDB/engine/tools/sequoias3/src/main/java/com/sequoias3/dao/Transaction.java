package com.sequoias3.dao;

import com.sequoias3.exception.S3ServerException;

public interface Transaction {
    void begin(ConnectionDao connection) throws S3ServerException;
    void commit(ConnectionDao connection) throws S3ServerException;
    void rollback(ConnectionDao connection);
}
