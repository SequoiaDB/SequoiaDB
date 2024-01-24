package com.sequoias3.exception;

public class S3UpdateUserException extends S3ServerException {
    private static final long serialVersionUID = -5238230425202542393L;

    public S3UpdateUserException(String message) {
        super(S3Error.USER_UPDATE_FAILED, message);
    }

    public S3UpdateUserException(String message, Throwable e) {
        super(S3Error.USER_UPDATE_FAILED, message, e);
    }
}
