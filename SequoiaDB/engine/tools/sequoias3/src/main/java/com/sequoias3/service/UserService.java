package com.sequoias3.service;

import com.sequoias3.model.AccessKeys;
import com.sequoias3.exception.S3ServerException;

public interface UserService {
    AccessKeys createUser(String newUserName, String role)
            throws S3ServerException;

    AccessKeys updateUser(String userName) throws S3ServerException;

    AccessKeys getUser(String userName) throws S3ServerException;

    void deleteUser(String username, Boolean forceDelete) throws S3ServerException;
}
