/*
 * Copyright 2018 SequoiaDB Inc.
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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.ResultSet;
import com.sequoiadb.message.request.GetMoreRequest;
import com.sequoiadb.message.request.KillContextRequest;
import com.sequoiadb.message.response.SdbReply;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.io.Closeable;

/**
 * Cursor of query result.
 */
public class DBCursor implements Closeable {
    private final Sequoiadb sequoiadb;
    private long contextId;
    private ResultSet resultSet;
    private boolean isClosed;
    private boolean isEOC;
    private boolean isStarted;
    private byte[] currentRaw;
    private BSONObject currentObj;
    private final int closeAllCursorMark;

    DBCursor(Sequoiadb sequoiadb, long contextId, ResultSet resultSet) {
        this.sequoiadb = sequoiadb;
        this.contextId = contextId;
        this.resultSet = resultSet;
        this.closeAllCursorMark = sequoiadb.getCloseAllCursorMark();
    }

    DBCursor(SdbReply response, Sequoiadb sequoiadb) {
        this(sequoiadb, response.getContextId(), response.getResultSet());
    }

    /**
     * Judge whether the next document exists or not.
     *
     * @return true for next data exists while false for not
     * @throws BaseException If error happens.
     */
    public boolean hasNext() throws BaseException {
        checkCloseAllCursor();

        if (isClosed) {
            throw new BaseException(SDBError.SDB_DMS_CONTEXT_IS_CLOSE, "The cursor has closed");
        }
        if (isEOC) {
            return false;
        }

        isStarted = true;

        if (resultSet != null && resultSet.hasNext()) {
            return true;
        }

        if (contextId != -1) {
            resultSet = getResultSetFromServer();
            return resultSet != null && resultSet.hasNext();
        }

        return false;
    }

    /**
     * Judge whether next raw data exists.
     *
     * @return true for next raw data exists while false for not
     * @throws BaseException If error happens.
     * @see #hasNext()
     * @deprecated always use DBCursor.hasNext()
     */
    @Deprecated
    public boolean hasNextRaw() throws BaseException {
        return hasNext();
    }

    /**
     * Get next document.
     * Calling this function after the cursor have been closed
     * will throw BaseException with error {@link SDBError#SDB_DMS_CONTEXT_IS_CLOSE}
     *
     * @return the next data or null if the cursor is empty
     * or the cursor is closed
     * @throws BaseException If error happens.
     */
    public BSONObject getNext() throws BaseException {
        if (hasNext()) {
            currentObj = resultSet.getNext();
            currentRaw = null;
            return currentObj;
        } else {
            return null;
        }
    }

    /**
     * Get raw data of next record.
     * Calling this function after the cursor have been closed
     * will throw BaseException with error {@link SDBError#SDB_DMS_CONTEXT_IS_CLOSE}
     *
     * @return a byte array of raw data of next record or null
     * if the cursor is empty
     * @throws BaseException If error happens.
     */
    public byte[] getNextRaw() throws BaseException {
        if (hasNext()) {
            currentRaw = resultSet.getNextRaw();
            currentObj = null;
            return currentRaw;
        } else {
            return null;
        }
    }

    /**
     * Get current document.
     * Calling this function after the cursor have been closed
     * will throw BaseException with error {@link SDBError#SDB_DMS_CONTEXT_IS_CLOSE}
     *
     * @return the current data or null if the cursor is empty
     * @throws BaseException If error happens.
     */
    public BSONObject getCurrent() throws BaseException {
        checkCloseAllCursor();

        if (isClosed) {
            throw new BaseException(SDBError.SDB_DMS_CONTEXT_IS_CLOSE, "The cursor has closed");
        }
        if (!isStarted) {
            return getNext();
        }

        if (currentObj != null) {
            return currentObj;
        }

        if (currentRaw != null) {
            currentObj = Helper.decodeBSONBytes(currentRaw);
            return currentObj;
        }

        return null;
    }

    private ResultSet getResultSetFromServer() {
        GetMoreRequest request = new GetMoreRequest(contextId, -1);
        SdbReply response = sequoiadb.requestAndResponse(request);

        int flag = response.getFlag();
        if (flag != 0) {
            contextId = -1;
            if (flag == SDBError.SDB_DMS_EOC.getErrorCode()) {
                isEOC = true;
                return null;
            }
            sequoiadb.throwIfError(response);
        }
        else if (-1 == response.getContextId()) {
            contextId = -1;
        }

        return response.getResultSet();
    }

    /**
     * Close the cursor.
     *
     * @throws BaseException If error happens.
     */
    @Override
    public void close() throws BaseException {
        if (!isClosed) {
            killContext();
            isClosed = true;
        }
    }

    private void killContext() {
        if (contextId == -1) {
            return;
        }

        long[] contextIds = new long[]{contextId};
        KillContextRequest request = new KillContextRequest(contextIds);
        SdbReply response = sequoiadb.requestAndResponse(request);
        sequoiadb.throwIfError(response);
        contextId = -1;
    }

    private void checkCloseAllCursor() {
        if (this.closeAllCursorMark < this.sequoiadb.getCloseAllCursorMark()) {
            // closeAllCursor() is called, so the current cursor object
            // needs to be marked as closed
            isClosed = true;
        }
    }
}
