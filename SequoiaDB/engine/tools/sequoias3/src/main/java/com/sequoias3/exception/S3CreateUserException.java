package com.sequoias3.exception;

public class S3CreateUserException extends S3ServerException {
    private static final long serialVersionUID = -5238230425202542393L;

    public S3CreateUserException(String message) {
        super(S3Error.USER_CREATE_FAILED, message);
    }

    public S3CreateUserException(String message, Throwable e) {
        super(S3Error.USER_CREATE_FAILED, message, e);
    }
}