/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.sequoiadb.message;

public final class MsgConstants {
    /* 
     * Internal flag, not exposed to users.
     * This flag identifies whether the inserted record (or batch of records) contains
     * the '_id' field, which can be used to skip the '_id' field check.
    */
    public final static int FLG_INSERT_HAS_ID_FIELD = 0x00000010;

    /* auth key*/
    public final static String AUTH_USER = "User";
    public final static String AUTH_PASSWD = "Passwd";
    public final static String AUTH_TEXT_PASSWD = "TextPasswd";
    public final static String AUTH_OPTIONS = "Options";
    public final static String AUTH_TYPE = "Type";
    public final static String AUTH_STEP = "Step";
    public final static String AUTH_SALT = "Salt";
    public final static String AUTH_ITERATION_COUNT = "IterationCount";
    public final static String AUTH_NONCE = "Nonce";
    public final static String AUTH_IDENTIFY = "Identify";
    public final static String AUTH_PROOF  = "Proof";

    /* auth value*/
    public final static int AUTH_SHA265_STEP_1 = 1;
    public final static int AUTH_SHA265_STEP_2 = 2;
    public final static int AUTH_TYPE_MD5_PWD = 0;
    public final static int CLIENT_NONCE_LEN = 24;
    public final static String AUTH_JAVA_IDENTIFY = "Java_Session";
    public final static String CLIENT_KEY = "Client Key";
    public final static String SERVER_KEY = "Server Key";
}
