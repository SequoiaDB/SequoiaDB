package com.sequoiadb.threadexecutor.exception;

public class SchException extends Exception {
    public SchException(String msg) {
        super(msg);
    }

    public SchException(String message, Throwable cause) {
        super(message, cause);
    }

    public SchException(Throwable cause) {
        super(cause);
    }
}
