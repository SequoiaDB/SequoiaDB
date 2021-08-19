package com.sequoias3.dao;

import com.sequoias3.core.Part;
import com.sequoias3.exception.S3ServerException;

public interface PartDao {
    void insertPart(ConnectionDao connection, long uploadId, long partNumber, Part part) throws S3ServerException;

    void updatePart(ConnectionDao connection, long uploadId, long partNumber, Part part) throws S3ServerException;

    Part queryPartByPartnumber(ConnectionDao connection, long uploadId, long partNumber) throws S3ServerException;

    Part queryPartBySize(ConnectionDao connection, long uploadId, Long size) throws S3ServerException;

    void deletePart(ConnectionDao connection, long uploadId, Long partNumber) throws S3ServerException;

    QueryDbCursor queryPartList(long uploadId, Boolean onlyPositiveNo,
                                Integer marker, Integer maxSize) throws S3ServerException;

    QueryDbCursor queryPartListForUpdate(ConnectionDao connection, long uploadId) throws S3ServerException;
}
