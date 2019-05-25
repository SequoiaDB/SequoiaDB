﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using SequoiaDB.Bson;

namespace SequoiaDB
{

    /** \class DBLob
     *  \brief Database operation interfaces of large object
     */
	public class  DBLob
	{
        /**
         *  \memberof SDB_LOB_SEEK_SET 0
         *  \brief Change the position from the beginning of lob 
         */
        public const int SDB_LOB_SEEK_SET = 0;
    
        /**
         *  \memberof SDB_LOB_SEEK_CUR 1
         *  \brief Change the position from the current position of lob 
         */
        public const int SDB_LOB_SEEK_CUR = 1;
    
        /**
         *  \memberof SDB_LOB_SEEK_END 2
         *  \brief Change the position from the end of lob 
         */
        public const int SDB_LOB_SEEK_END = 2;

        /**
         *  \memberof SDB_LOB_CREATEONLY 0x00000001
         *  \brief Open a new lob only
         */
        internal const int SDB_LOB_CREATEONLY = 0x00000001;

        /**
         *  \memberof SDB_LOB_READ 0x00000004
         *  \brief LOB open mode for reading
         */
        public const int SDB_LOB_READ = 0x00000004;

        /**
         *  \memberof SDB_LOB_WRITE 0x00000008
         *  \brief LOB open mode for writing
         */
        public const int SDB_LOB_WRITE = 0x00000008;

        // the max lob data size to send for one message
        private const int SDB_LOB_MAX_WRITE_DATA_LENGTH = 2097152; // 2M;
        private const int SDB_LOB_WRITE_DATA_LENGTH = 524288; // 512k;
        private const int SDB_LOB_READ_DATA_LENGTH = 65536; // 64k;

        private const int SDB_LOB_ALIGNED_LEN = 524288; // 512k
        private const int FLG_LOBOPEN_WITH_RETURNDATA = 0x00000002;
    
        private const long SDB_LOB_DEFAULT_OFFSET  = -1;
        private const int SDB_LOB_DEFAULT_SEQ = 0;
    
        private DBCollection _cl = null;
        private IConnection  _connection = null;
        internal bool        _isBigEndian = false;

        private ObjectId     _id;
        private int          _mode;
        private int          _pageSize;
        private long         _lobSize;
        private long         _createTime;
        private long         _modificationTime;
        private long         _currentOffset;
        private long         _cachedOffset;
        private ByteBuffer   _cachedDataBuff;
        private bool         _isOpened;
        private bool         _seekWrite;
        // when first open/create DBLob, sequoiadb return the contextID for the
        // further reading/writing/close
        private long         _contextID;

        internal DBLob(DBCollection cl)
        {
            this._cl = cl;
            this._connection = cl.CollSpace.SequoiaDB.Connection;
            this._isBigEndian = cl.isBigEndian;
            _id = ObjectId.Empty;
            _mode = -1;
            _lobSize = 0;
            _createTime = 0;
            _currentOffset = -1;
            _cachedOffset = -1;
            _cachedDataBuff = null;
            _isOpened = false;
            _seekWrite = false;
            _contextID = -1;
        }

        /** \fn         Open( ObjectId id, int mode )
         * \brief       Open an exist lob, or create a lob
         * \param       id   the lob's id
         * \param       mode available mode is SDB_LOB_CREATEONLY or SDB_LOB_READ.
         *              SDB_LOB_CREATEONLY 
         *                  create a new lob with given id, if id is null, it will 
         *                  be generated in this function;
         *              SDB_LOB_READ
         *                  read an exist lob
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        internal void Open(ObjectId id, int mode)
        {
            // check
            if (_isOpened)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_HAS_OPEN, "lob have opened: id = " + _id);
            }
            if (mode != SDB_LOB_CREATEONLY && 
                mode != SDB_LOB_READ &&
                mode != SDB_LOB_WRITE)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "mode is unsupported: " + mode);
            }
            if (mode == SDB_LOB_READ || mode == SDB_LOB_WRITE)
            {
                if (id == ObjectId.Empty)
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "id must be specify"
                    + " in mode:" + mode);
                }
            }
            // gen oid
            _id = id;
            if (mode == SDB_LOB_CREATEONLY)
            {
                if (_id == ObjectId.Empty)
                {
                    _id = ObjectId.GenerateNewId();
                }
            }
            // mode
            _mode = mode;
            _currentOffset = 0;
            // open
            _Open();
            _isOpened = true;
        }

        /** \fn          Close()
          * \brief       Close the lob
          * \return void
          * \exception SequoiaDB.BaseException
          * \exception System.Exception
          */
        public void Close()
        {
            if (!_isOpened)
            {
                return;
            }

            ByteBuffer request = _GenerateCloseLobRequest();
            ByteBuffer respone = _SendAndReiveMessage(request);

            SDBMessage retInfo = SDBMessageHelper.ExtractReply(respone);
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_CLOSE_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int errorCode = retInfo.Flags;
            if (0 != errorCode)
            {
                throw new BaseException(errorCode);
            }
            // update the last update time
            List<BsonDocument> resultSet = retInfo.ObjectList;
            if (resultSet != null && resultSet.Count > 0)
            {
                BsonDocument obj = resultSet[0];

                if (obj != null && obj.Contains(SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME))
                {
                    if (obj[SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME].IsInt64)
                    {
                        _modificationTime = obj[SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME].AsInt64;
                    }
                    else
                    {
                        throw new BaseException((int)Errors.errors.SDB_SYS,
                            "the received data is not a long type.");
                    }
                }
            }
            _isOpened = false;
        }

        /** \fn          Read( byte[] b )
         *  \brief       Reads up to b.length bytes of data from this 
         *               lob into an array of bytes. 
         *  \param       b   the buffer into which the data is read.
         *  \return      the total number of bytes read into the buffer, or
         *               <code>-1</code> if there is no more data because the end of
         *               the file has been reached, or <code>0<code> if 
         *               <code>b.length</code> is Zero.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public int Read(byte[] b)
        {
            return Read(b, 0, b.Length);
        }

        /** \fn          Read(byte[] b, int off, int len)
         *  \brief       Reads up to <code>len</code> bytes of data from this lob into
         *               an array of bytes.
         *  \param       b   the buffer into which the data is read.
         *  \param       off the start offset in the destination array <code>b</code>.
         *  \param       len the maximum number of bytes read.
         *  \return      the total number of bytes read into the buffer, or
         *               <code>-1</code> if there is no more data because the end of
         *               the file has been reached, or <code>0<code> if 
         *               <code>b.length</code> is Zero.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public int Read(byte[] b, int off, int len)
        {
            if (!_isOpened)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (b == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "b is null");
            }

            if (len < 0 || len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid len");
            }

            if (off < 0 || off > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid off");
            }

            if (off + len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "off + len is great than b.Length");
            }

            if (b.Length == 0)
            {
                return 0;
            }
            return _Read(b, off, len);
        }

        /** \fn          Write( byte[] b )
         *  \brief       Writes b.length bytes from the specified 
         *               byte array to this lob. 
         *  \param       b   The data.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Write(byte[] b)
        {
            Write(b, 0, b.Length);
        }

        /** \fn          Write(byte[] b, int off, int len)
         *  \brief       Writes len bytes from the specified 
         *               byte array to this lob. 
         *  \param       b   The data.
         *  \param       off   The offset of the data buffer.
         *  \param       len   The lenght to write.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Write(byte[] b, int off, int len)
        {
            if (!_isOpened)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (b == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "input is null");
            }

            if (len < 0 || len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid len");
            }

            if (off < 0 || off > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid off");
            }

            if (off + len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "off + len is great than b.length");
            }
            int offset = off;
            int leftLen = len;
            int writeLen = 0;
            while (leftLen > 0)
            {
                writeLen = (leftLen < SDB_LOB_MAX_WRITE_DATA_LENGTH) ?
                        leftLen : SDB_LOB_MAX_WRITE_DATA_LENGTH;
                _Write(b, offset, writeLen);
                leftLen -= writeLen;
                offset += writeLen;
            }
        }

        /** \fn          void Seek( long size, int seekType )
         *  \brief       Change the read/write position of the lob. The new position is 
         *               obtained by adding <code>size</code> to the position 
         *               specified by <code>seekType</code>. If <code>seekType</code> 
         *               is set to SDB_LOB_SEEK_SET, SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, 
         *               the offset is relative to the start of the lob, the current 
         *               position of lob, or the end of lob.
         *  \param       size the adding size.
         *  \param       seekType  SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Seek(long size, int seekType)
        {
            if (!_isOpened)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (_mode != SDB_LOB_READ && 
                _mode != SDB_LOB_CREATEONLY && 
                _mode != SDB_LOB_WRITE)
            {
                throw new BaseException((int)Errors.errors.SDB_OPTION_NOT_SUPPORT, "seek() is not supported"
                        + " in mode=" + _mode);
            }

            if (SDB_LOB_SEEK_SET == seekType)
            {
                if (size < 0 || (size > _lobSize && _mode == SDB_LOB_READ))
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "out of bound, lobSize=" + _lobSize);
                }

                _currentOffset = size;
            }
            else if (SDB_LOB_SEEK_CUR == seekType)
            {
                if ((_currentOffset + size >= _lobSize && _mode == SDB_LOB_READ)
                        || (_currentOffset + size < 0))
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, 
                        "out of bound, _currentOffset=" + _currentOffset + ", lobSize=" + _lobSize);
                }

                _currentOffset += size;
            }
            else if (SDB_LOB_SEEK_END == seekType)
            {
                if (size < 0 || (size > _lobSize && _mode == SDB_LOB_READ))
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "out of bound, lobSize=" + _lobSize);
                }

                _currentOffset = _lobSize - size;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "unreconigzed seekType: " + seekType);
            }

            if (_mode == SDB_LOB_CREATEONLY || _mode == SDB_LOB_WRITE)
            {
                _seekWrite = true;
            }
        }

        /** \fn          void Lock(long offset, long length)
         *  \brief       Lock LOB section for writing.
         *  \param       offset Lock start position.
         *  \param       length Lock length, -1 means lock to the end of lob.
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Lock(long offset, long length)
        {
            if (!_isOpened)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (offset < 0 || length < -1 || length == 0)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG,
                    "out of bound, offset=" + offset + ", length=" + length);
            }

            if (_mode != SDB_LOB_WRITE)
            {
                return;
            }

            ByteBuffer request = _GenerateLockLobRequest(_contextID, offset, length);
            ByteBuffer respone = _SendAndReiveMessage(request);

            SDBMessage retInfo = SDBMessageHelper.ExtractReply(respone);
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_LOCK_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int flag = retInfo.Flags;
            if (0 != flag)
            {
                throw new BaseException(flag);
            }
        }

        /** \fn          void LockAndSeek(long offset, long length)
         *  \brief       Lock LOB section for writing and seek to the offset position.
         *  \param       offset Lock start position.
         *  \param       length Lock length, -1 means lock to the end of lob.
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void LockAndSeek(long offset, long length)
        {
            Lock(offset, length);
            Seek(offset, SDB_LOB_SEEK_SET);
        }

        /** \fn          bool IsClosed()
         *  \brief       Test whether lob has been closed or not
         *  \return      true for lob has been closed, false for not
         */
        public bool IsClosed()
        {
            return !_isOpened;
        }

        /** \fn          ObjectId GetID()
         *  \brief       Get the lob's id
         *  \return      the lob's id
         */
        public ObjectId GetID()
        {
            return _id;
        }

        /** \fn          long GetSize()
         *  \brief       Get the size of lob
         *  \return      the lob's size
         */
        public long GetSize()
        {
            return _lobSize;
        }

        /** \fn          long GetCreateTime()
         *  \brief       get the create time of lob
         *  \return The lob's create time
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetCreateTime()
        { 
            return _createTime;
        }

        /** \fn          long GetModificationTime()
         *  \brief       get the last modification time of lob
         *  \return The lob's last modification time
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetModificationTime()
        {
            return _modificationTime;
        }

        /************************************** private methond **************************************/

        private void _Open()
        {
            /*
                /// open reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            // add info into object
            BsonDocument openLob = new BsonDocument();
            openLob.Add(SequoiadbConstants.FIELD_COLLECTION, _cl.FullName);
            openLob.Add(SequoiadbConstants.FIELD_LOB_OID, _id);
            openLob.Add(SequoiadbConstants.FIELD_LOB_OPEN_MODE, _mode);

            // set return data flag
            int flags = (_mode == SDB_LOB_READ) ? FLG_LOBOPEN_WITH_RETURNDATA :
                SequoiadbConstants.DEFAULT_FLAGS;

            // generate request
            ByteBuffer request = _GenerateOpenLobRequest(openLob, flags);

            // send and receive message
            ByteBuffer respone = _SendAndReiveMessage(request);

            // extract info from the respone
            SDBMessage retInfo = SDBMessageHelper.ExtractLobOpenReply(respone);

            // check the respone opcode
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_OPEN_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }

            // check the result
            int rc = retInfo.Flags;
            if (rc != 0)
            {
                throw new BaseException(rc);
            }
            /// get lob's meta info returned from engine
            List<BsonDocument> objList = retInfo.ObjectList;
            if (objList.Count() != 1)
            {
                throw new BaseException((int)Errors.errors.SDB_NET_BROKEN_MSG, 
                    "expect 1 record, but get " + objList.Count() + "records");
            }
            BsonDocument obj = objList[0];
            if (obj == null)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "expect 1 record, but we get null");
            }
            // lob size
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_SIZE) && obj[SequoiadbConstants.FIELD_LOB_SIZE].IsInt64)
            {
                _lobSize = obj[SequoiadbConstants.FIELD_LOB_SIZE].AsInt64;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "the received data is not a long type.");
            }
            // lob create time
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_CREATE_TIME) && obj[SequoiadbConstants.FIELD_LOB_CREATE_TIME].IsInt64)
            {
                _createTime = obj[SequoiadbConstants.FIELD_LOB_CREATE_TIME].AsInt64;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "the received data is not a long type.");
            }
            // last modified time
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME))
            {
                if (obj[SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME].IsInt64)
                {
                    _modificationTime = obj[SequoiadbConstants.FIELD_LOB_MODIFICATION_TIME].AsInt64;
                }
                else
                {
                    throw new BaseException((int)Errors.errors.SDB_SYS, "the received data is not a long type.");
                }
            }
            else
            {
                _modificationTime = _createTime;
            }
            // page size
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_PAGESIZE) && obj[SequoiadbConstants.FIELD_LOB_PAGESIZE].IsInt32)
            {
                _pageSize = obj[SequoiadbConstants.FIELD_LOB_PAGESIZE].AsInt32;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            // cache data
            _cachedDataBuff = retInfo.LobCachedDataBuf;
            if (_cachedDataBuff != null)
            {
                _currentOffset = 0;
                _cachedOffset = _currentOffset;
            }
            retInfo.LobCachedDataBuf = null;
            // contextID
            _contextID = retInfo.ContextIDList[0];
        }

        private int _Read(byte[] b, int off, int len)
        {
            int offset = off;
            int needRead = len;
            int onceRead = 0;
            int totalRead = 0;

            // when no data for return
            if (_currentOffset >= _lobSize)
            {
                return -1;
            }
            while (needRead > 0 && _currentOffset < _lobSize)
            {
                onceRead = _OnceRead(b, offset, needRead);
                if (onceRead == -1)
                {
                    if (totalRead == 0)
                    {
                        totalRead = -1;
                    }
                    // when we finish reading, let's stop
                    break;
                }
                offset += onceRead;
                needRead -= onceRead;
                totalRead += onceRead;
                onceRead = 0;
            }
            return totalRead;
        }

        private void _Write(byte[] input, int off, int len)
        {
            if (len == 0)
            {
                return;
            }
            long offset = _seekWrite ? _currentOffset : -1;
            ByteBuffer request = _GenerateWriteLobRequest(input, off, len, offset);
            // send and receive msg
            ByteBuffer respone = _SendAndReiveMessage(request);
            // extract info from return msg
            SDBMessage retInfo = SDBMessageHelper.ExtractReply(respone);

            if (retInfo.OperationCode != Operation.MSG_BS_LOB_WRITE_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int flag = retInfo.Flags;
            if (0 != flag)
            {
                throw new BaseException(flag);
            }
            _currentOffset += len;
            _lobSize = Math.Max(_lobSize, _currentOffset);
            _seekWrite = false;
        }

        private int _OnceRead(byte[] buf, int off, int len)
        {
            int needRead = len;
            int totalRead = 0;
            int onceRead = 0;
            int alignedLen = 0;

            // try to get data from local needRead
            if (_HasDataCached())
            {
                onceRead = _ReadInCache(buf, off, needRead);
                totalRead += onceRead;
                needRead -= onceRead;
                _currentOffset += onceRead;
                return totalRead;
            }

            // get data from database
            _cachedOffset = -1;
            _cachedDataBuff = null;

            // page align
            alignedLen = _ReviseReadLen(needRead);
            // build read message
            ByteBuffer request = _GenerateReadLobRequest(alignedLen);
            // seed and receive message to engine
            ByteBuffer respone = _SendAndReiveMessage(request);
            // extract return message
            SDBMessage retInfo = SDBMessageHelper.ExtractLobReadReply(respone);
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_READ_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int rc = retInfo.Flags;
            // meet the end of the lob
            if (rc == (int)Errors.errors.SDB_EOF)
            {
                return -1;
            }
            if (rc != 0)
            {
                throw new BaseException(rc);
            }
            // sanity check
            // return message is |MsgOpReply|_MsgLobTuple|data|
            int retMsgLen = retInfo.RequestLength;
            if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                    SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "invalid message's length: " + retMsgLen);
            }
            long offsetInEngine = retInfo.LobOffset;
            if (_currentOffset != offsetInEngine)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "local read offset(" + _currentOffset +
                                ") is not equal with what we expect(" + offsetInEngine + ")");
            }
            int retLobLen = (int)retInfo.LobLen;
            if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                    SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH + retLobLen)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "invalid message's length: " + retMsgLen);
            }
            /// get return data
            _cachedDataBuff = retInfo.LobCachedDataBuf;
            retInfo.LobCachedDataBuf = null;
            // sanity check 
            int remainLen = _cachedDataBuff.Remaining();
            if (remainLen != retLobLen)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "the remaining in buffer(" + remainLen +
                        ") is not equal with what we expect(" + retLobLen + ")");
            }
            // if what we got is more than what we expect,
            // let's cache some for next reading request.
            int copy = (needRead < retLobLen) ? needRead : retLobLen;
            int output = _cachedDataBuff.PopByteArray(buf, off, copy);
            if (copy != output)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                    string.Format("Invalid bytes length in the cached buffer, copy {0} bytes, actually output {1} bytes", copy, output));
            }
            totalRead += output;
            _currentOffset += output;
            if (needRead < retLobLen)
            {
                _cachedOffset = _currentOffset;
            }
            else
            {
                _cachedOffset = -1;
                _cachedDataBuff = null;
            }
            return totalRead;
        }

        private bool _HasDataCached()
        {
            int remaining = (_cachedDataBuff != null) ? _cachedDataBuff.Remaining() : 0;
            return (_cachedDataBuff != null && 0 < remaining &&
                    0 <= _cachedOffset &&
                    _cachedOffset <= _currentOffset &&
                    _currentOffset < (_cachedOffset + remaining));
        }

        private int _ReadInCache(byte[] buf, int off, int needRead)
        {
            if (needRead > buf.Length - off)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "buf size is to small");
            }
            int readInCache = (int)(_cachedOffset + _cachedDataBuff.Remaining() - _currentOffset);
            readInCache = readInCache <= needRead ? readInCache : needRead;
            // if we had used "lobSeek" to adjust "_readOffset",
            // let's adjust the right place to copy data
            if (_currentOffset > _cachedOffset)
            {
                int currentPos = _cachedDataBuff.Position();
                int newPos = currentPos + (int)(_currentOffset - _cachedOffset);
                _cachedDataBuff.Position(newPos);
            }
            // copy the data from cache out to the buf for user
            int outputNum = _cachedDataBuff.PopByteArray(buf, off, readInCache);
            if (readInCache != outputNum)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                   string.Format("Invalid bytes length in the cached buffer, copy {0} bytes, actually output {1} bytes", readInCache, outputNum));
            }
            if (_cachedDataBuff.Remaining() == 0)
            {
                // TODO: shell we need to reuse the ByteBuffer ?
                _cachedDataBuff = null;
            }
            else
            {
                _cachedOffset = _currentOffset + readInCache;
            }
            return readInCache;
        }

        private int _ReviseReadLen(int needLen)
        {
            int mod = (int)(_currentOffset & (_pageSize - 1));
            // when "needLen" is great than (2^31 - 1) - 3,
            // alignedLen" will be less than 0, but we should not worry
            // about this, because before we finish using the cached data,
            // we won't come here, at that moment, "alignedLen" will be not be less
            // than "needLen"
            int alignedLen = Helper.RoundToMultipleXLength(needLen, SDB_LOB_ALIGNED_LEN);
            if (alignedLen < needLen)
            {
                alignedLen = SDB_LOB_ALIGNED_LEN;
            }
            alignedLen -= mod;
            if (alignedLen < SDB_LOB_ALIGNED_LEN)
            {
                alignedLen += SDB_LOB_ALIGNED_LEN;
            }
            return alignedLen;
        }

        // TODO: need to put these functions to SDBMessageHelper.cs ?
        private ByteBuffer _GenerateOpenLobRequest(BsonDocument openLob, int flags)
        {
            /*
                /// open reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            byte[] openLobBytes = openLob.ToBson();

            // get the total length of buffer we need
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH +
                Helper.RoundToMultipleXLength(openLobBytes.Length, 4);

            // alloc buffer
            ByteBuffer totalBuff = new ByteBuffer(totalLen);
            if (_isBigEndian)
            {
                totalBuff.IsBigEndian = true;
                // keep the openLobBytes save in big endian
                SDBMessageHelper.BsonEndianConvert(openLobBytes, 0, openLobBytes.Length, true);
            }

            // MsgHeader
            SDBMessageHelper.AddMsgHeader(totalBuff, totalLen, 
                (int)Operation.MSG_BS_LOB_OPEN_REQ, SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(totalBuff,SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W,(short)0,flags,
                SequoiadbConstants.DEFAULT_CONTEXTID,openLobBytes.Length);

            // meta
            SDBMessageHelper.AddBytesToByteBuffer(totalBuff, openLobBytes, 0, openLobBytes.Length, 4);

            return totalBuff;
        }

        private ByteBuffer _GenerateCloseLobRequest()
        {
            /*
                /// close reg msg is |MsgOpLob|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH;

            // add _MsgOpLob into buff with convert(db.endianConvert)
            ByteBuffer buff = new ByteBuffer(SDBMessageHelper.MESSAGE_OPLOB_LENGTH);
            buff.IsBigEndian = _isBigEndian;

            // MsgHeader
            SDBMessageHelper.AddMsgHeader(buff, totalLen,
                    (int)Operation.MSG_BS_LOB_CLOSE_REQ,
                    SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                    SequoiadbConstants.DEFAULT_W, (short)0,
                    SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);
            return buff;
        }

        private ByteBuffer _GenerateReadLobRequest(int length)
        {
            /*
                /// read req msg is |MsgOpLob|_MsgLobTuple|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;

                union _MsgLobTuple
                {
                   struct
                   {
                      UINT32 len ;
                      UINT32 sequence ;
                      SINT64 offset ;
                   } columns ;

                   CHAR data[16] ;
                } ;
             */
            // total length of the message
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                    + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH;

            // new byte buffer
            ByteBuffer buff = new ByteBuffer(totalLen);
            buff.IsBigEndian = _isBigEndian;

            // add MsgHeader
            SDBMessageHelper.AddMsgHeader(buff, totalLen,
                (int)Operation.MSG_BS_LOB_READ_REQ,
                SequoiadbConstants.ZERO_NODEID, 0);

            // add MsgOpLob
            SDBMessageHelper.AddLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short)0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

            // add MsgLobTuple
            AddMsgTuple(buff, length, SDB_LOB_DEFAULT_SEQ, _currentOffset);

            return buff;
        }

        private ByteBuffer _GenerateWriteLobRequest(byte[] input, int off, int len, long lobOffset)
        {
            /*
                /// write req msg is |MsgOpLob|_MsgLobTuple|data|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;

                union _MsgLobTuple
                {
                   struct
                   {
                      UINT32 len ;
                      UINT32 sequence ;
                      SINT64 offset ;
                   } columns ;

                   CHAR data[16] ;
                } ;
             */
            if (input == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "input is null");
            }
            if (off + len > input.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "off + len is more than input.length");
            }
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                    + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH
                    + Helper.RoundToMultipleXLength(len, 4);
            // alloc ByteBuffer
            ByteBuffer totalBuf = new ByteBuffer(totalLen);
            totalBuf.IsBigEndian = _isBigEndian;
            
            // MsgHeader
            SDBMessageHelper.AddMsgHeader(totalBuf, totalLen,
                    (int)Operation.MSG_BS_LOB_WRITE_REQ,
                    SequoiadbConstants.ZERO_NODEID, 0);
            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(totalBuf, SequoiadbConstants.DEFAULT_VERSION,
                    SequoiadbConstants.DEFAULT_W, (short)0,
                    SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

            // MsgLobTuple
            AddMsgTuple(totalBuf, len, SDB_LOB_DEFAULT_SEQ, lobOffset);

            // lob data
            SDBMessageHelper.AddBytesToByteBuffer(totalBuf, input, off, len, 4);

            return totalBuf;
        }

        private ByteBuffer _GenerateLockLobRequest(long contextId, long offset, long length)
        {
            /*
                /// lock lob reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            BsonDocument meta = new BsonDocument();
            meta.Add(SequoiadbConstants.FIELD_LOB_OFFSET, offset);
            meta.Add(SequoiadbConstants.FIELD_LOB_LENGTH, length);
            byte[] metaInfoBytes = meta.ToBson();

            // get the total length of buffer we need
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH +
                Helper.RoundToMultipleXLength(metaInfoBytes.Length, 4);

            // alloc buffer
            ByteBuffer totalBuff = new ByteBuffer(totalLen);
            if (_isBigEndian)
            {
                totalBuff.IsBigEndian = true;
                // keep the metaInfoBytes save in big endian
                SDBMessageHelper.BsonEndianConvert(metaInfoBytes, 0, metaInfoBytes.Length, true);
            }

            // MsgHeader
            SDBMessageHelper.AddMsgHeader(totalBuff, totalLen,
                (int)Operation.MSG_BS_LOB_LOCK_REQ, SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(totalBuff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short)0, SequoiadbConstants.DEFAULT_FLAGS,
                contextId, metaInfoBytes.Length);

            // meta
            SDBMessageHelper.AddBytesToByteBuffer(totalBuff, metaInfoBytes, 0, metaInfoBytes.Length, 4);

            return totalBuff;
        }

        private void AddMsgTuple(ByteBuffer buff, int length, int sequence,
                                 long offset)
        {
            buff.PushInt(length);
            buff.PushInt(sequence);
            buff.PushLong(offset);
        }

        private ByteBuffer _SendAndReiveMessage(ByteBuffer request)
        {
            if (request == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "request can't be null");
            }

            _connection.SendMessage(request.ByteArray());
            return _connection.ReceiveMessage2(_isBigEndian);
        }

	}
}
