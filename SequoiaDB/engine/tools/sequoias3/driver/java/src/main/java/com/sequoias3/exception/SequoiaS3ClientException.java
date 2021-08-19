package com.sequoias3.exception;

public class SequoiaS3ClientException extends Exception{

    public SequoiaS3ClientException(String errorMessage) {
        super(errorMessage);
    }

    public SequoiaS3ClientException(String message, Throwable t) {
        super(message, t);
    }
}
