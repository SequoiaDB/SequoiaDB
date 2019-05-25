/*
 * Copyright 2017 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.exception;

import java.util.Arrays;

/**
 * Base exception of SequoiaDB.
 */
public class BaseException extends RuntimeException {

    private static final long serialVersionUID = -6115487863398926195L;

    private int errcode;
    private SDBError error;
    private String detail;


    /**
     * @param error  The enumeration object of sequoiadb error.
     * @param detail The error detail.
     * @param e      The exception used to build exception chain.
     * @since 2.8
     */
    public BaseException(SDBError error, String detail, Throwable e) {
        super(e);
        this.errcode = error.getErrorCode();
        this.error = error;
        this.detail = detail;
    }

    /**
     * @param error  The enumeration object of sequoiadb error.
     * @param detail The error detail.
     * @since 2.8
     */
    public BaseException(SDBError error, String detail) {
        this.errcode = error.getErrorCode();
        this.error = error;
        this.detail = detail;
    }

    /**
     * @param error The enumeration object of sequoiadb error.
     * @param e     The exception used to build exception chain.
     * @since 2.8
     */
    public BaseException(SDBError error, Throwable e) {
        this(error, e.getMessage(), e);
    }

    /**
     * @param error The enumeration object of sequoiadb error.
     * @since 2.8
     */
    public BaseException(SDBError error) {
        this(error, (String) null);
    }


    /**
     * @param errCode The error code return by engine.
     * @since 2.8
     */
    public BaseException(int errCode) {
        this(errCode, (String) null);
    }


    /**
     * @param errCode The error code return by engine.
     * @param detail  The error detail.
     * @since 2.8
     */
    public BaseException(int errCode, String detail) {
        this.errcode = errCode;
        this.error = SDBError.getSDBError(errCode);
        this.detail = detail;
    }

    /**
     * @param errorType The error type.
     * @deprecated
     */
    public BaseException(String errorType, Object... objs) {
        this(SDBError.valueOf(errorType), Arrays.toString(objs));
    }

    /**
     * @param errorCode The error code return by engine.
     * @deprecated
     */
    public BaseException(int errorCode, Object... objs) {
        this(errorCode, Arrays.toString(objs));
    }


    /**
     * Get the error message.
     *
     * @return The error message.
     */
    @Override
    public String getMessage() {
        if (detail != null && !detail.isEmpty()) {
            if (error != null) {
                return error.toString() + ", detail: " + detail;
            } else {
                return getErrorType() + "(" + errcode + "), detail: " + detail;
            }
        } else if (error != null) {
            return error.toString();
        } else {
            return getErrorType() + "(" + errcode + ")";
        }
    }

    /**
     * Get the error type.
     *
     * @return The error type.
     */
    public String getErrorType() {
        return error != null ? error.getErrorType() : "SDB_UNKNOWN";
    }

    /**
     * Get the error code.
     *
     * @return The error code.
     */
    public int getErrorCode() {
        return errcode;
    }
}
