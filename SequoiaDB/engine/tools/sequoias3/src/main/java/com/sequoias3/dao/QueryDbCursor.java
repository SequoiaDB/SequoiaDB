package com.sequoias3.dao;

import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

public interface QueryDbCursor {
    public boolean hasNext() throws S3ServerException;
    public BSONObject getNext() throws S3ServerException;
    public BSONObject getCurrent() throws S3ServerException;
}
