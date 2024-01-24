/**
 * Copyright (c) 2017, SequoiaDB Ltd.
 * File Name:OperateException.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2017-2-24上午10:56:28
 *  @version 1.00
 */
package com.sequoiadb.exception;

public class OperateException extends ReliabilityException {
    /**
     * 
     */
    private static final long serialVersionUID = 1303611435830754145L;

    public OperateException( String message ) {
        super( message );
    }

    public OperateException() {
        super();
    }

    public OperateException( String message, Throwable cause ) {
        super( message, cause );
    }

    public OperateException( Throwable cause ) {
        super( cause );
    }
}
