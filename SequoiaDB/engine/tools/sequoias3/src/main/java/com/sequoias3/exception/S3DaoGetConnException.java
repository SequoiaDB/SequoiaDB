package com.sequoias3.exception;

public class S3DaoGetConnException extends S3ServerException {
    private static final long serialVersionUID = -5238230425202542393L;

    public S3DaoGetConnException(String message) {
        super(S3Error.DAO_GETCONN_ERROR, message);
    }

    public S3DaoGetConnException(String message, Throwable e) {
        super(S3Error.DAO_GETCONN_ERROR, message, e);
    }
}