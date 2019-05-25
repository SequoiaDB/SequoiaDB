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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.ResultSet;
import com.sequoiadb.message.request.*;
import com.sequoiadb.message.response.LobOpenResponse;
import com.sequoiadb.message.response.LobReadResponse;
import com.sequoiadb.message.response.SdbReply;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

class DBLobImpl implements DBLob {
    final static String FIELD_NAME_LOB_OPEN_MODE = "Mode";
    final static String FIELD_NAME_LOB_OID = "Oid";
    final static String FIELD_NAME_LOB_SIZE = "Size";
    final static String FIELD_NAME_LOB_CREATE_TIME = "CreateTime";
    final static String FIELD_NAME_LOB_MODIFICATION_TIME = "ModificationTime";
    final static String FIELD_NAME_LOB_PAGESIZE = "LobPageSize";
    final static String FIELD_NAME_LOB_LENGTH = "Length";
    final static int SDB_LOB_CREATEONLY = 0x00000001;

    private final static int SDB_LOB_MAX_WRITE_DATA_LENGTH = 2097152; // 2M;
    private final static int SDB_LOB_WRITE_DATA_LENGTH = 524288; // 512k;
    private final static int SDB_LOB_READ_DATA_LENGTH = 65536; // 64k;

    private final static int SDB_LOB_ALIGNED_LEN = 524288; // 512k;
    private final static int FLG_LOBOPEN_WITH_RETURNDATA = 0X00000002;

    private Sequoiadb _sdb;
    private DBCollection _cl;
    private ObjectId _id;
    private int _mode;
    private int _pageSize;
    private long _lobSize;
    private long _createTime;
    private long _modificationTime;
    private long _currentOffset = 0;
    private long _cachedOffset = -1;
    private ByteBuffer _cachedDataBuff = null;
    private boolean _isOpened = false;
    private boolean _seekWrite = false;

    /* when first open/create DBLob, sequoiadb return the contextID for the
     * further reading/writing/close
    */
    private long _contextID;

    /**
     * @param cl The instance of DBCollection
     * @throws BaseException If error happens.
     */
    DBLobImpl(DBCollection cl) throws BaseException {
        if (cl == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "cl is null");
        }

        _cl = cl;
        _sdb = cl.getSequoiadb();
    }

    /**
     * Create a lob, lob's id will auto generate in this function.
     *
     * @param id the lob's id
     * @throws BaseException If error happens.
     *                       public void open() {
     *                       open(null, SDB_LOB_CREATEONLY);
     *                       }
     *                       <p>
     *                       /**
     *                       Open an existing lob with id.
     * @throws BaseException If error happens.
     */
    public void open(ObjectId id) {
        open(id, SDB_LOB_READ);
    }

    /**
     * Open an existing lob, or create a lob.
     *
     * @param id   the lob's id
     * @param mode available mode is SDB_LOB_CREATEONLY or SDB_LOB_READ.
     *             SDB_LOB_CREATEONLY
     *             create a new lob with given id, if id is null, it will
     *             be generated in this function;
     *             SDB_LOB_READ
     *             read an exist lob
     * @throws BaseException If error happens.
     */
    public void open(ObjectId id, int mode) throws BaseException {
        if (_isOpened) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "lob have opened: id = " + _id);
        }

        if (mode != SDB_LOB_CREATEONLY &&
                mode != SDB_LOB_READ &&
                mode != SDB_LOB_WRITE) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "mode is unsupported: " + mode);
        }

        if (mode == SDB_LOB_READ || mode == SDB_LOB_WRITE) {
            if (null == id) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "id must be specify"
                        + " in mode:" + mode);
            }
        }

        _id = id;
        if (mode == SDB_LOB_CREATEONLY) {
            if (null == _id) {
                _id = ObjectId.get();
            }
        }

        _mode = mode;
        _currentOffset = 0;

        _open();
        _isOpened = true;
    }

    private void _open() throws BaseException {
        BSONObject openLob = new BasicBSONObject();
        openLob.put(SdbConstants.FIELD_COLLECTION, _cl.getFullName());
        openLob.put(FIELD_NAME_LOB_OID, _id);
        openLob.put(FIELD_NAME_LOB_OPEN_MODE, _mode);

        int flags = (_mode == SDB_LOB_READ) ? FLG_LOBOPEN_WITH_RETURNDATA : 0;

        LobOpenRequest request = new LobOpenRequest(openLob, flags);
        LobOpenResponse response = _sdb.requestAndResponse(request, LobOpenResponse.class);
        _sdb.throwIfError(response, openLob);

        BSONObject obj = response.getMetaInfo();
        _lobSize = (Long) obj.get(FIELD_NAME_LOB_SIZE);
        _createTime = (Long) obj.get(FIELD_NAME_LOB_CREATE_TIME);
        if (obj.containsField(FIELD_NAME_LOB_MODIFICATION_TIME)) {
            _modificationTime = (Long) obj.get(FIELD_NAME_LOB_MODIFICATION_TIME);
        } else {
            _modificationTime = _createTime;
        }
        _pageSize = (Integer) obj.get(FIELD_NAME_LOB_PAGESIZE);
        _cachedDataBuff = response.getData();
        if (_cachedDataBuff != null) {
            _currentOffset = 0;
            _cachedOffset = _currentOffset;
        }

        _contextID = response.getContextId();
    }

    /**
     * Get the lob's id.
     *
     * @return the lob's id
     */
    @Override
    public ObjectId getID() {
        return _id;
    }

    /**
     * Get the size of lob.
     *
     * @return the lob's size
     */
    @Override
    public long getSize() {
        return _lobSize;
    }

    /**
     * Get the create time of lob.
     *
     * @return the lob's create time
     */
    @Override
    public long getCreateTime() {
        return _createTime;
    }

    /**
     * Get the last modification time of lob.
     *
     * @return the lob's last modification time
     */
    @Override
    public long getModificationTime() {
        return _modificationTime;
    }

    /**
     * Close the lob.
     *
     * @throws BaseException If error happens.
     */
    @Override
    public void close() throws BaseException {
        if (!_isOpened) {
            return;
        }

        LobCloseRequest request = new LobCloseRequest(_contextID);
        SdbReply response = _sdb.requestAndResponse(request);
        _sdb.throwIfError(response);
        _isOpened = false;
        if (response.getReturnedNum() > 0) {
            ResultSet resultSet = response.getResultSet();
            BSONObject obj = resultSet.getNext();
            if (obj != null && obj.containsField(FIELD_NAME_LOB_MODIFICATION_TIME)) {
                _modificationTime = (Long) obj.get(FIELD_NAME_LOB_MODIFICATION_TIME);
            }
        }
    }

    /**
     * Write bytes from the input stream to this lob.
     *
     * @param in the input stream.
     * @throws BaseException If error happens.
     */
    @Override
    public void write(InputStream in) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }
        if (in == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "input is null");
        }
        int readNum = 0;
        byte[] tmpBuf = new byte[SDB_LOB_WRITE_DATA_LENGTH];
        try {
            while (-1 < (readNum = in.read(tmpBuf))) {
                write(tmpBuf, 0, readNum);
            }
        } catch (IOException e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        }
    }

    /**
     * Write <code>b.length</code> bytes from the specified
     * byte array to this lob.
     *
     * @param b the data.
     * @throws BaseException If error happens.
     */
    @Override
    public void write(byte[] b) throws BaseException {
        write(b, 0, b.length);
    }

    /**
     * Write <code>len</code> bytes from the specified
     * byte array starting at offset <code>off</code> to this lob.
     *
     * @param b   the data.
     * @param off the start offset in the data.
     * @param len the number of bytes to write.
     * @throws BaseException If error happens.
     */
    @Override
    public void write(byte[] b, int off, int len) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (b == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "input is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "off + len is great than b.length");
        }
        int offset = off;
        int leftLen = len;
        int writeLen = 0;
        while (leftLen > 0) {
            writeLen = (leftLen < SDB_LOB_MAX_WRITE_DATA_LENGTH) ?
                    leftLen : SDB_LOB_MAX_WRITE_DATA_LENGTH;
            _write(b, offset, writeLen);
            leftLen -= writeLen;
            offset += writeLen;
        }

    }

    /**
     * Read data from this lob into the output stream.
     *
     * @param out the output stream.
     * @throws BaseException If error happens.
     */
    @Override
    public void read(OutputStream out) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (out == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "output stream is null");
        }
        int readNum = 0;
        byte[] tmpBuf = new byte[SDB_LOB_READ_DATA_LENGTH];
        while (-1 < (readNum = read(tmpBuf, 0, tmpBuf.length))) {
            try {
                out.write(tmpBuf, 0, readNum);
            } catch (IOException e) {
                throw new BaseException(SDBError.SDB_SYS, e);
            }
        }
    }

    /**
     * Read up to <code>b.length</code> bytes of data from this
     * lob into an array of bytes.
     *
     * @param b the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or
     * <code>-1</code> if there is no more data because the end of
     * the file has been reached, or <code>0<code> if
     * <code>b.length</code> is Zero.
     * @throws BaseException If error happens.
     */
    @Override
    public int read(byte[] b) throws BaseException {
        return read(b, 0, b.length);
    }

    /**
     * Read up to <code>len</code> bytes of data from this lob into
     * an array of bytes.
     *
     * @param b   the buffer into which the data is read.
     * @param off the start offset in the destination array <code>b</code>.
     * @param len the maximum number of bytes read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     * there is no more data because the end of the file has been
     * reached, or <code>0</code> if <code>len</code> is Zero.
     * @throws BaseException If error happens.
     */
    @Override
    public int read(byte[] b, int off, int len) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (b == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "b is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "off + len is great than b.length");
        }

        if (b.length == 0) {
            return 0;
        }
        return _read(b, off, len);
    }

    /**
     * Change the read or write position of the lob.
     * The new position is obtained by adding <code>size</code> to the position
     * specified by <code>seekType</code>. If <code>seekType</code>
     * is set to SDB_LOB_SEEK_SET, SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END,
     * the offset is relative to the start of the lob, the current
     * position of lob, or the end of lob.
     *
     * @param size     the adding size.
     * @param seekType SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @throws BaseException If error happens.
     */
    @Override
    public void seek(long size, int seekType) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (_mode != SDB_LOB_READ &&
                _mode != SDB_LOB_CREATEONLY &&
                _mode != SDB_LOB_WRITE) {
            throw new BaseException(SDBError.SDB_OPTION_NOT_SUPPORT, "seek() is not supported"
                    + " in mode=" + _mode);
        }

        if (SDB_LOB_SEEK_SET == seekType) {
            if (size < 0 || (size >= _lobSize && _mode == SDB_LOB_READ)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "out of bound, lobSize=" + _lobSize);
            }

            _currentOffset = size;
        } else if (SDB_LOB_SEEK_CUR == seekType) {
            if ((_currentOffset + size >= _lobSize && _mode == SDB_LOB_READ)
                    || (_currentOffset + size < 0)) {
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        "out of bound, _currentOffset=" + _currentOffset + ", lobSize=" + _lobSize);
            }

            _currentOffset += size;
        } else if (SDB_LOB_SEEK_END == seekType) {
            if (size < 0 || (size > _lobSize && _mode == SDB_LOB_READ)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "out of bound, lobSize=" + _lobSize);
            }

            _currentOffset = _lobSize - size;
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, "unreconigzed seekType: " + seekType);
        }

        if (_mode == SDB_LOB_CREATEONLY || _mode == SDB_LOB_WRITE) {
            _seekWrite = true;
        }
    }

    /**
     * Lock LOB section for writing.
     *
     * @param offset lock start position
     * @param length lock length, -1 means lock to the end of lob
     * @throws BaseException If error happens..
     */
    @Override
    public void lock(long offset, long length) throws BaseException {
        if (!_isOpened) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (offset < 0 || length < -1 || length == 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG,
                    "out of bound, offset=" + offset + ", length=" + length);
        }

        if (_mode != SDB_LOB_WRITE) {
            return;
        }

        LobLockRequest request = new LobLockRequest(_contextID, offset, length);
        SdbReply response = _sdb.requestAndResponse(request);
        _sdb.throwIfError(response);
    }

    /**
     * Lock LOB section for writing and seek to the offset position.
     *
     * @param offset lock start position
     * @param length lock length, -1 means lock to the end of lob
     * @throws BaseException If error happens..
     */
    @Override
    public void lockAndSeek(long offset, long length) throws BaseException {
        lock(offset, length);
        seek(offset, SDB_LOB_SEEK_SET);
    }

    private int _reviseReadLen(int needLen) {
        int mod = (int) (_currentOffset & (_pageSize - 1));
        int alignedLen = Helper.alignedSize(needLen, SDB_LOB_ALIGNED_LEN);
        if (alignedLen < needLen) {
            alignedLen = SDB_LOB_ALIGNED_LEN;
        }
        alignedLen -= mod;
        if (alignedLen < SDB_LOB_ALIGNED_LEN) {
            alignedLen += SDB_LOB_ALIGNED_LEN;
        }
        return alignedLen;
    }

    private boolean _hasDataCached() {
        int remaining = (_cachedDataBuff != null) ? _cachedDataBuff.remaining() : 0;
        return (_cachedDataBuff != null && 0 < remaining &&
                0 <= _cachedOffset &&
                _cachedOffset <= _currentOffset &&
                _currentOffset < (_cachedOffset + remaining));
    }

    private int _readInCache(byte[] buf, int off, int needRead) {
        if (needRead > buf.length - off) {
            throw new BaseException(SDBError.SDB_SYS, "buf size is to small");
        }
        int readInCache = (int) (_cachedOffset + _cachedDataBuff.remaining() - _currentOffset);
        readInCache = readInCache <= needRead ? readInCache : needRead;
        if (_currentOffset > _cachedOffset) {
            int currentPos = _cachedDataBuff.position();
            int newPos = currentPos + (int) (_currentOffset - _cachedOffset);
            _cachedDataBuff.position(newPos);
        }
        _cachedDataBuff.get(buf, off, readInCache);
        if (_cachedDataBuff.remaining() == 0) {
            _cachedDataBuff = null;
        } else {
            _cachedOffset = _currentOffset + readInCache;
        }
        return readInCache;
    }

    private int _onceRead(byte[] buf, int off, int len) {
        int needRead = len;
        int totalRead = 0;
        int onceRead = 0;
        int alignedLen = 0;

        if (_hasDataCached()) {
            onceRead = _readInCache(buf, off, needRead);
            totalRead += onceRead;
            needRead -= onceRead;
            _currentOffset += onceRead;
            return totalRead;
        }

        _cachedOffset = -1;
        _cachedDataBuff = null;

        alignedLen = _reviseReadLen(needRead);

        LobReadRequest request = new LobReadRequest(_contextID, alignedLen, _currentOffset);
        LobReadResponse response = _sdb.requestAndResponse(request, LobReadResponse.class);

        int rc = response.getFlag();
        if (rc == SDBError.SDB_EOF.getErrorCode()) {
            return -1; // meet the end of the lob
        } else if (rc != 0) {
            _sdb.throwIfError(response);
        }

        long offsetInEngine = response.getOffset();
        if (_currentOffset != offsetInEngine) {
            throw new BaseException(SDBError.SDB_SYS,
                    "local read offset(" + _currentOffset +
                            ") is not equal with what we expect(" + offsetInEngine + ")");
        }

        int retLobLen = response.getLobLen();

        _cachedDataBuff = response.getData();
        int remainLen = _cachedDataBuff.remaining();
        if (remainLen != retLobLen) {
            throw new BaseException(SDBError.SDB_SYS, "the remaining in buffer(" + remainLen +
                    ") is not equal with what we expect(" + retLobLen + ")");
        }

        if (needRead < retLobLen) {
            _cachedDataBuff.get(buf, off, needRead);
            totalRead += needRead;
            _currentOffset += needRead;
            _cachedOffset = _currentOffset;
        } else {
            _cachedDataBuff.get(buf, off, retLobLen);
            totalRead += retLobLen;
            _currentOffset += retLobLen;
            _cachedOffset = -1;
            _cachedDataBuff = null;
        }
        return totalRead;
    }

    private int _read(byte[] b, int off, int len) {
        int offset = off;
        int needRead = len;
        int onceRead = 0;
        int totalRead = 0;
        if (_currentOffset == _lobSize) {
            return -1;
        }
        while (needRead > 0 && _currentOffset < _lobSize) {
            onceRead = _onceRead(b, offset, needRead);
            if (onceRead == -1) {
                if (totalRead == 0) {
                    totalRead = -1;
                }
                break;
            }
            offset += onceRead;
            needRead -= onceRead;
            totalRead += onceRead;
            onceRead = 0;
        }
        return totalRead;
    }

    private void _write(byte[] input, int off, int len) throws BaseException {
        if (len == 0) {
            return;
        }

        long offset = _seekWrite ? _currentOffset : -1;
        LobWriteRequest request = new LobWriteRequest(_contextID, input, off, len, offset);
        SdbReply response = _sdb.requestAndResponse(request);
        _sdb.throwIfError(response);
        _currentOffset += len;
        _lobSize = Math.max(_lobSize, _currentOffset);
        _seekWrite = false;
    }
}
