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

import java.io.Closeable;
import java.io.InputStream;
import java.io.OutputStream;

import org.bson.BSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.exception.BaseException;

/**
 * LOB of SequoiaDB.
 */
public interface DBLob extends Closeable {
    /**
     * Change the position from the beginning of lob
     */
    int SDB_LOB_SEEK_SET = 0;

    /**
     * Change the position from the current position of lob
     */
    int SDB_LOB_SEEK_CUR = 1;

    /**
     * Change the position from the end of lob
     */
    int SDB_LOB_SEEK_END = 2;

    /**
     * LOB open mode for reading
     */
    int SDB_LOB_READ = 0x00000004;

    /**
     * LOB open mode for writing
     */
    int SDB_LOB_WRITE = 0x00000008;

    /**
     * LOB open mode for share read
     */
    int SDB_LOB_SHAREREAD = 0x00000040;

    /**
     * Get the lob's id.
     *
     * @return the lob's id
     */
    ObjectId getID();

    /**
     * Get the size of lob.
     *
     * @return the lob's size
     */
    long getSize();

    /**
     * Get the create time of lob.
     *
     * @return the lob's create time
     */
    long getCreateTime();

    /**
     * Get the last modification time of lob.
     *
     * @return the lob's last modification time
     */
    long getModificationTime();

    /**
     * Write bytes from the input stream to this lob.
     * user need to close the input stream
     *
     * @param in the input stream.
     * @throws BaseException If error happens.
     */
    void write(InputStream in) throws BaseException;

    /**
     * Write <code>b.length</code> bytes from the specified
     * byte array to this lob.
     *
     * @param b the data.
     * @throws BaseException If error happens.
     */
    void write(byte[] b) throws BaseException;

    /**
     * Write <code>len</code> bytes from the specified
     * byte array starting at offset <code>off</code> to this lob.
     *
     * @param b   the data.
     * @param off the start offset in the data.
     * @param len the number of bytes to write.
     * @throws BaseException If error happens.
     */
    void write(byte[] b, int off, int len) throws BaseException;

    /**
     * Read the content to the output stream.
     * user need to close the output stream.
     *
     * @param out the output stream.
     * @throws BaseException If error happens.
     */
    void read(OutputStream out) throws BaseException;

    /**
     * Read up to <code>b.length</code> bytes of data from this lob into
     * an array of bytes.
     *
     * @param b the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     * there is no more data because the end of the file has been
     * reached, or <code>0</code> if <code>b.length</code> is Zero.
     * @throws BaseException If error happens.
     */
    int read(byte[] b) throws BaseException;

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
    int read(byte[] b, int off, int len) throws BaseException;

    /**
     * Change the read or write position of the lob.
     * The new position is obtained by adding size to the position specified by
     * seekType. If seekType is set to SDB_LOB_SEEK_SET,
     * SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, the offset is
     * relative to the start of the lob, the current position
     * of lob, or the end of lob.
     *
     * @param size     the adding size.
     * @param seekType SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @throws BaseException If error happens..
     */
    void seek(long size, int seekType) throws BaseException;

    /**
     * Lock LOB section for write mode.
     *
     * @param offset lock start position
     * @param length lock length, -1 means lock to the end of lob
     * @throws BaseException If error happens..
     */
    void lock(long offset, long length) throws BaseException;

    /**
     * Lock LOB section for write mode and seek to the offset position.
     *
     * @param offset lock start position
     * @param length lock length, -1 means lock to the end of lob
     * @throws BaseException If error happens..
     */
    void lockAndSeek(long offset, long length) throws BaseException;

    /**
     * Close the lob.
     *
     * @throws BaseException
     *             If error happens.
     */
    @Override
    void close() throws BaseException;

    /**
     * Check whether current offset has reached to the max size of the current lob.
     *
     * @return Return true if yes while false for not.
     */
    boolean isEof();

    /**
     * Get the run time detail information of lob.
     *
     * @return Return detail of runtime information.
     */
    BSONObject getRunTimeDetail() throws BaseException;
}

