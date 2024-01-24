/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.common.metadata;

import com.sequoiadb.flink.common.exception.SDBException;

/**
 * ExtraRowKind is an extension to Flink {@link org.apache.flink.types.RowKind},
 * added UPDATE_PK_BEF, UPDATE_PK_AFT to represent "primary key update" changelog.
 */
public enum ExtraRowKind {
    INSERT(0),
    UPDATE_AFT(1),
    DELETE(2),
    UPDATE_PK_BEF(3),
    UPDATE_PK_AFT(4),

    ;

    private final int code;

    ExtraRowKind(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    /**
     * transfer code(integer) to ExtraRowKind enum.
     * as definition above,
     *  0 -> INSERT
     *  1 -> UPDATE_AFT
     *  2 -> DELETE
     *  3 -> UPDATE_PK_BEF
     *  4 -> UPDATE_PK_AFT
     *
     * @param code
     * @return
     */
    public static ExtraRowKind from(Integer code) {
        ExtraRowKind kind = null;
        switch (code) {
            case 0:
                kind = INSERT;
                break;
            case 1:
                kind = UPDATE_AFT;
                break;
            case 2:
                kind = DELETE;
                break;
            case 3:
                kind = UPDATE_PK_BEF;
                break;
            case 4:
                kind = UPDATE_PK_AFT;
                break;

            default:
                // LOG.warn, fix-me: dump to error changelogs file.
                throw new SDBException(String.format(
                        "unsupported extra row kind, code: %s",
                        code));
        }

        return kind;
    }
}
