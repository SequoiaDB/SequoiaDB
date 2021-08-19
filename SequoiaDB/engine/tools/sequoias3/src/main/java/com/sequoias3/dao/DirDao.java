package com.sequoias3.dao;

import com.sequoiadb.base.DBCursor;
import com.sequoias3.core.Dir;
import com.sequoias3.core.Region;
import com.sequoias3.exception.S3ServerException;

public interface DirDao {
    void insertDir(ConnectionDao connection, String metaCsName, Dir dir, Region region) throws S3ServerException;

    Dir queryDir(ConnectionDao connection, String metaCsName, Long bucketId,
                          String delimiter, String dirName, Boolean forUpdate)
            throws S3ServerException;

    QueryDbCursor queryDirList(String metaCsName, Long bucketId,
                          String delimiter, String dirPrefix, String startAfter)
            throws S3ServerException;

    void delete(ConnectionDao connectionDao, String metaCsName, Long bucketId,
                String delimiter, String dirName)
            throws S3ServerException;
}
