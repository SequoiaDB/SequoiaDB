package com.sequoias3.exception;

public class S3ServerException extends Exception {
    private static final long serialVersionUID = 5375282751760263614L;
    private S3Error error;

    public S3ServerException(S3Error error, String message, Throwable e) {
        super(message, e);
        this.error = error;
    }

    public S3ServerException(S3Error error, String message) {
        super(message);
        this.error = error;
    }

    public S3Error getError() {
        return error;
    }

    @Override
    public String toString() {
        return super.toString() + ", errorCode=" + error.getErrIndex();
    }
}
