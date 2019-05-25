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

package com.sequoiadb.message;

/**
 * @since 2.9
 */
public final class MsgOpCode {
    private MsgOpCode() {
    }

    public static final int INVALID_OP_CODE = 0;

    public static final int RESP_MASK = 0x80000000;

    public static final int UPDATE_REQ = 2001;
    public static final int UPDATE_RESP = RESP_MASK | UPDATE_REQ;

    public static final int INSERT_REQ = 2002;
    public static final int INSERT_RESP = RESP_MASK | INSERT_REQ;

    public static final int SQL_REQ = 2003;
    public static final int SQL_RESP = RESP_MASK | SQL_REQ;

    public static final int QUERY_REQ = 2004;
    public static final int QUERY_RESP = RESP_MASK | QUERY_REQ;

    public static final int GET_MORE_REQ = 2005;
    public static final int GET_MORE_RESP = RESP_MASK | GET_MORE_REQ;

    public static final int DELETE_REQ = 2006;
    public static final int DELETE_RESP = RESP_MASK | DELETE_REQ;

    public static final int KILL_CONTEXT_REQ = 2007;
    public static final int KILL_CONTEXT_RESP = RESP_MASK | KILL_CONTEXT_REQ;

    public static final int DISCONNECT = 2008;
    public static final int INTERRUPT = 2009;

    public static final int TRANSACTION_BEGIN_REQ = 2010;
    public static final int TRANSACTION_BEGIN_RESP = RESP_MASK | TRANSACTION_BEGIN_REQ;

    public static final int TRANSACTION_COMMIT_REQ = 2011;
    public static final int TRANSACTION_COMMIT_RESP = RESP_MASK | TRANSACTION_COMMIT_REQ;

    public static final int TRANSACTION_ROLLBACK_REQ = 2012;
    public static final int TRANSACTION_ROLLBACK_RESP = RESP_MASK | TRANSACTION_ROLLBACK_REQ;

    public static final int AGGREGATE_REQ = 2019;
    public static final int AGGREFATE_RESP = RESP_MASK | AGGREGATE_REQ;

    public static final int AUTH_VERIFY_REQ = 7000;
    public static final int AUTH_VERIFY_RESP = RESP_MASK | AUTH_VERIFY_REQ;

    public static final int AUTH_CREATE_USER_REQ = 7001;
    public static final int AUTH_CREATE_USER_RESP = RESP_MASK | AUTH_CREATE_USER_REQ;

    public static final int AUTH_DELETE_USER_REQ = 7002;
    public static final int AUTH_DELETE_USER_RESP = RESP_MASK | AUTH_DELETE_USER_REQ;

    public static final int LOB_OPEN_REQ = 8001;
    public static final int LOB_OPEN_RESP = RESP_MASK | LOB_OPEN_REQ;

    public static final int LOB_WRITE_REQ = 8002;
    public static final int LOB_WRITE_RESP = RESP_MASK | LOB_OPEN_REQ;

    public static final int LOB_READ_REQ = 8003;
    public static final int LOB_READ_RESP = RESP_MASK | LOB_READ_REQ;

    public static final int LOB_REMOVE_REQ = 8004;
    public static final int LOB_REMOVE_RESP = RESP_MASK | LOB_REMOVE_REQ;

    public static final int LOB_UPDATE_REQ = 8005;
    public static final int LOB_UPDATE_RESP = RESP_MASK | LOB_UPDATE_REQ;

    public static final int LOB_CLOSE_REQ = 8006;
    public static final int LOB_CLOSE_RESP = RESP_MASK | LOB_CLOSE_REQ;

    public static final int LOB_LOCK_REQ = 8007;
    public static final int LOB_LOCK_RESP = RESP_MASK | LOB_LOCK_REQ;

    public static final int LOB_TRUNCATE_REQ = 8008;
    public static final int LOB_TRUNCATE_RESP = RESP_MASK | LOB_TRUNCATE_REQ;

    public static final int SYS_INFO_REQ = 0xFFFFFFFF;
    public static final int SYS_INFO_RESP = 0xFFFFFFFF;
}