package com.sequoias3.dao;

import com.sequoias3.model.Owner;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3ServerException;

public interface UserDao {
    void insertUser(User user) throws S3ServerException;

    void deleteUser(String userName) throws S3ServerException;

    void updateUserKeys(String userName, String accessKeyId, String secretAccessKey)
            throws S3ServerException;

    User getUserByName(String userName) throws S3ServerException;

    User getUserByAccessKeyID(String accessKeyID) throws S3ServerException;

    Owner getOwnerByUserID(long userId) throws S3ServerException;
}
