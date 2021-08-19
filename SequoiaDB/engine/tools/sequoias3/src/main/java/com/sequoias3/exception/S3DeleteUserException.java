package com.sequoias3.exception;

public class S3DeleteUserException extends S3ServerException {
    private static final long serialVersionUID = -5238230425202542393L;

    public S3DeleteUserException(String message) {
        super(S3Error.USER_DELETE_FAILED, message);
    }

    public S3DeleteUserException(String message, Throwable e) {
        super(S3Error.USER_DELETE_FAILED, message, e);
    }
}
