package com.sequoias3.dao;

import com.sequoias3.exception.S3ServerException;

public interface IDGeneratorDao {
    Long getNewId(int type) throws S3ServerException;

    void insertId(int type) throws S3ServerException;
}
