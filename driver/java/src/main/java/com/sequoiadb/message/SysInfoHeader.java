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
public abstract class SysInfoHeader {
    protected static final int SYS_INFO_SPECIAL_LEN = 0xFFFFFFFF;
    protected static final int SYS_INFO_EYE_CATCHER = 0xFFFEFDFC;
    protected static final int SYS_INFO_EYE_CATCHER_REVERT = 0xFCFDFEFF;
    protected static final int HEADER_LENGTH = 12;
    protected int specialSysInfoLen;
    protected int eyeCatcher;
    protected int realMsgLen;

    protected SysInfoHeader() {
        specialSysInfoLen = SYS_INFO_SPECIAL_LEN;
        eyeCatcher = SYS_INFO_EYE_CATCHER;
        realMsgLen = HEADER_LENGTH;
    }
}
