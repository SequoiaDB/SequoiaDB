/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:ReliabilityBaseException.java
 * 类的详细描述
 *
 *  @author wenjingwang
 * Date:2017-2-24上午10:23:57
 *  @version 1.00
 */
package com.sequoiadb.exception;

import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.SftpException;

import java.io.FileNotFoundException;
import java.io.IOException;

public class ReliabilityException extends Exception {
    /**
     * 
     */
    private static final long serialVersionUID = 6219985388657559901L;

    public enum ExceptionType {
        SSHEXCEPTION(1), DBEXCEPTION(2), FILEEXCEPTION(3), IOEXCEPTION(4) {
        };

        private int value;

        private ExceptionType( int value ) {
            this.value = value;
        }

        public int getValue() {
            return this.value;
        }
    };

    ExceptionType type;

    public void setExceptionType( Throwable cause ) {
        if ( cause instanceof BaseException ) {
            this.type = ExceptionType.DBEXCEPTION;
        } else if ( cause instanceof SftpException ) {
            this.type = ExceptionType.SSHEXCEPTION;
        } else if ( cause instanceof JSchException ) {
            this.type = ExceptionType.SSHEXCEPTION;
        } else if ( cause instanceof IOException ) {
            this.type = ExceptionType.IOEXCEPTION;
        } else if ( cause instanceof FileNotFoundException ) {
            this.type = ExceptionType.FILEEXCEPTION;
        } else if ( cause instanceof ReliabilityException ) {
            this.type = ( ( ReliabilityException ) cause ).getExceptionType();
        }
    }

    public ExceptionType getExceptionType() {
        return this.type;
    }

    public ReliabilityException( String message ) {
        super( message );
    }

    public ReliabilityException() {
        super();
    }

    public ReliabilityException( String message, Throwable cause ) {
        super( message, cause );
    }

    public ReliabilityException( Throwable cause ) {
        super( cause );
        setExceptionType( cause );
    }
}
