package com.sequoias3.dao;

import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.Grant;

import java.util.List;

public interface AclDao {
    void insertAcl(ConnectionDao connectionDao, long aclId, Grant grant) throws S3ServerException;

    void deleteAcl(ConnectionDao connectionDao, long aclId) throws S3ServerException;

    List<Grant> queryAcl(long aclId) throws S3ServerException;
}
