using System;
using System.IO;

namespace SequoiaDB
{
    /// <summary>
    /// Reference java ByteBuffer.
    /// Only for SequoiaDB CSharp Driver to use.
    /// </summary>
    public class ByteBuffer
    {
        private int _mark = -1;
        private int _position = 0;
        private int _limit;
        private int _capacity;

        private byte[] _byte_array;
        // Default is Little Endian
        private bool _isBigEndian = false;
        /// <summary>
        /// Get and set the endian info.
        /// </summary>
        public bool IsBigEndian
        {
            get { return _isBigEndian; }
            set { _isBigEndian = value; }
        }

        private void _Initialize(int capacity)
        {
            this._capacity = capacity;
            this._byte_array = new byte[capacity];
            this.Clear();
        }

        /// <summary>
        /// Constructor: Build a new buffer with the specified capacity.
        /// <param name="capacity">The capacity of ByteBuffer</param>
        /// </summary>
        public ByteBuffer(int capacity)
        {
            this._Initialize(capacity);
        }

        /// <summary>
        /// Constructor: Wrap or Copy a byte array into a buffer
        /// </summary>
        /// <param name="bytes"></param>
        public ByteBuffer(byte[] bytes, bool wrap)
        {
            if (wrap)
            {
                this._byte_array = bytes;
                this._capacity = bytes.Length;
                this._limit = bytes.Length;
                this._mark = -1;
                this._position = 0;
            }
            else
            {
                this._Initialize(bytes.Length);
                this.PushByteArray(bytes);
                this.Flip();
            }
        }

        /// <summary>
        /// Constructor: Copy an existing Byte Array to ByteBuffer.
        /// </summary>
        /// <param name="bytes">The source byte array</param>
        public ByteBuffer(byte[] bytes):this(bytes,false)
        {
        }

        /// <summary>
        /// Return the current psotion of ByteBuffer
        /// </summary>
        public int Position()
        {
            return _position;
        }

        /// <summary>
        /// Set the new position of the current ByteBuffer
        /// </summary>
        /// <param name="newPosition"></param>
        /// <returns></returns>
        public ByteBuffer Position(int newPosition)
        {
            if (newPosition > _limit || newPosition < 0)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG,
                    string.Format("The new position{0} is more than the limit{1}, or less than 0. ", newPosition, _limit));
            }
            _position = newPosition;
            if (_mark > _position)
            {
                _mark = -1;
            }
            return this;
        }

        /// <summary>
        /// Return the capacity of ByteBuffer
        /// </summary>
        public int Capacity()
        {
            return _capacity;
        }

        /// <summary>
        /// Return the limit of the ByteBuffer
        /// </summary>
        /// <returns></returns>
        public int Limit()
        {
            return _limit;
        }

        /// <summary>
        /// Set the limit of the ByteBuffer
        /// </summary>
        /// <param name="newLimit"></param>
        /// <returns></returns>
        public ByteBuffer limit(int newLimit)
        {
            if (newLimit > _capacity || newLimit < 0)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG,
                    string.Format("The new limit{0} is more than the capacity{1}, or less than 0.", newLimit, _capacity));
            }
            _limit = newLimit;
            if (_position > _limit)
            {
                _position = _limit;
            }
            if (_mark > _limit)
            {
                _mark = -1;
            }
            return this;
        }

        /// <summary>
        /// Tells whether there are any elements between the current position and the limit.
        /// </summary>
        /// <returns></returns>
        public bool HasRemaining()
        {
            return _position < _limit;
        }

        /// <summary>
        ///  Returns the number of elements between the current position and the limit.
        /// </summary>
        /// <returns></returns>
        public int Remaining()
        {
            return _limit - _position;
        }

        public ByteBuffer Flip()
        {
            _limit = _position;
            _position = 0;
            _mark = -1;
            return this;
        }

        public ByteBuffer Rewind()
        {
            _position = 0;
            _mark = -1;
            return this;
        }

        public ByteBuffer Clear()
        {
            _position = 0;
            _limit = _capacity;
            _mark = -1;
            return this;
        }

        public ByteBuffer Mark()
        {
            _mark = _position;
            return this;
        }

        public ByteBuffer Reset()
        {
            int m = _mark;
            if (m < 0)
                throw new BaseException((int)Errors.errors.SDB_SYS,
                    string.Format("Invalid value of mark: {0}", m));
            _position = m;
            return this;
        }

        /// <summary>
        /// Return reference of the Byte Array.
        /// </summary>
        /// <returns></returns>
        public byte[] ByteArray()
        {
            return _byte_array;
        }

        /// <summary>
        /// Return the Copy of the Byte Array.
        /// </summary>
        /// <returns>Byte[]</returns>
        public byte[] ToByteArray()
        {
            byte[] return_array = new byte[_capacity];
            _byte_array.CopyTo(return_array, 0);
            return return_array;
        }

        /// <summary>
        /// Push a byte into ByteBuffer
        /// </summary>
        /// <param name="by">One Byte</param>
        public void PushByte(byte by)
        {
            EnsureCapacity(1);
            _byte_array[_position++] = by;
        }

        /// <summary>
        /// Push a byte array into ByteBuffer
        /// </summary>
        /// <param name="bytes"></param>
        /// <param name="off"></param>
        /// <param name="len"></param>
        public void PushByteArray(byte[] bytes, int off, int len)
        {
            EnsureCapacity(len);
            Array.Copy(bytes, off, _byte_array, _position, len);
            _position += len;
        }

        /// <summary>
        /// Push a byte array into ByteBuffer
        /// </summary>
        /// <param name="bytes">Byte Array</param>
        public void PushByteArray(byte[] bytes)
        {
            PushByteArray(bytes, 0, bytes.Length);
        }

        /// <summary>
        /// Push a short integer into ByteBuffer
        /// </summary>
        /// <param name="num">Short Integer</param>
        public void PushShort(short num)
        {
            byte[] tmp = BitConverter.GetBytes(num);
            if (_isBigEndian)
                Array.Reverse(tmp);
            PushByteArray(tmp);
        }

        /// <summary>
        /// Push a integer into ByteBuffer
        /// </summary>
        /// <param name="num">Integer</param>
        public void PushInt(int num)
        {
            byte[] tmp = BitConverter.GetBytes(num);
            if (_isBigEndian)
                Array.Reverse(tmp);
            PushByteArray(tmp);
        }

        /// <summary>
        /// Push a long integer into ByteBuffer
        /// </summary>
        /// <param name="num">Long Integer</param>
        public void PushLong(long num)
        {
            byte[] tmp = BitConverter.GetBytes(num);
            if (_isBigEndian)
                Array.Reverse(tmp);
            PushByteArray(tmp);
        }

        /// <summary>
        /// Pop a byte from ByteBuffer and current_position plus 1
        /// </summary>
        /// <returns>One Byte</returns>
        public byte PopByte()
        {
            if (Remaining() < 1)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "No byte for getting.");
            }
            byte ret = _byte_array[_position++];
            return ret;
        }

        /// <summary>
        /// Pop a short integer from ByteBuffer and current_position plus 2
        /// </summary>
        /// <returns>Short Integer</returns>
        public short PopShort()
        {
            // overflow
            if (Remaining() < 2)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "No short for getting.");
            }
            byte[] tmp = new byte[2];
            for (int i = 0; i < 2; i++)
                tmp[i] = _byte_array[_position++];
            if (_isBigEndian)
                Array.Reverse(tmp);
            return BitConverter.ToInt16(tmp, 0);
        }

        /// <summary>
        /// Pop a integer from ByteBuffer and current_position plus 4
        /// </summary>
        /// <returns>Integer</returns>
        public int PopInt()
        {
            // overflow
            if (Remaining() < 4)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "No int for getting.");
            }
            byte[] tmp = new byte[4];
            for (int i = 0; i < 4; i++)
                tmp[i] = _byte_array[_position++];
            if (_isBigEndian)
                Array.Reverse(tmp);
            return BitConverter.ToInt32(tmp, 0);
        }

        /// <summary>
        /// Pop a long integer from ByteBuffer and current_position plus 8
        /// </summary>
        /// <returns>Long Integer</returns>
        public long PopLong()
        {
            // overflow
            if (Remaining() < 8)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "No long for getting.");
            }
            byte[] tmp = new byte[8];
            for (int i = 0; i < 8; i++)
                tmp[i] = _byte_array[_position++];
            if (_isBigEndian)
                Array.Reverse(tmp);
            return BitConverter.ToInt64(tmp, 0);
        }

        /// <summary>
        /// Pop a byte array with length Length from ByteBuffer
        /// </summary>
        /// <param name="length">The length of byte array</param>
        /// <returns>Byte Array</returns>
        public byte[] PopByteArray(int length)
        {
            // overflow
            if (Remaining() < length)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "No enough  bytes for getting.");
            }
            byte[] ret = new byte[length];
            Array.Copy(_byte_array, _position, ret, 0, length);
            _position += length;
            return ret;
        }
        /// <summary>
        /// Pop a byte array with length Length from ByteBuffer
        /// </summary>
        /// <param name="bytes"></param>
        /// <param name="index"></param>
        /// <param name="len"></param>
        /// <returns>
        /// The length of bytes which has copy. for 0 is return,
        /// means there has no bytes in the ByteBuffer.
        /// </returns>
        public int PopByteArray(byte[] bytes, int index, int len)
        {
            int remaining = Remaining();
            if (remaining == 0)
            {
                return 0;
            }
            int copyLen = len <= remaining ? len : remaining;
            Array.Copy(_byte_array, _position, bytes, index, copyLen);
            _position += copyLen;
            return copyLen;
        }

        /** return an int value with a single one-bit, in the position
         *  of the highest-order one-bit in the specified value, or zero if
         *  the specified value is itself equal to zero.
         *  when the specified value is negative, return int.MinValue.
         */
        private static int HighestOneBit(int i)
        {
            if (i < 0)
            {
                return int.MinValue;
            }
            // see Integer::highestOneBit in java for more detail
            // HD, Figure 3-1
            i |= (i >>  1);
            i |= (i >>  2);
            i |= (i >>  4);
            i |= (i >>  8);
            i |= (i >> 16);
            return i - (i >> 1);
        }

        /**
         * Normalizes the specified capacity of the buffer to power of 2, which is
         * often helpful for optimal memory usage and performance.
         * If it is greater than or equal to Integer.MAX_VALUE, it returns Integer.MAX_VALUE.
         * If it is zero, it returns zero.
         */
        private static int NormalizeCapacity(int capacity)
        {
            if (capacity < 0)
            {
                return int.MaxValue;
            }
            int newCapacity = HighestOneBit(capacity);
            newCapacity <<= (newCapacity < capacity ? 1 : 0);
            return newCapacity < 0 ? int.MaxValue : newCapacity;
        }

        private void Expand(int newCapacity)
        {
            if (newCapacity > _capacity)
            {
                byte[] tmp_buf = new byte[newCapacity];
                tmp_buf.Initialize();
                _byte_array.CopyTo(tmp_buf, 0);
                _capacity = newCapacity;
                _limit = newCapacity;
                _byte_array = tmp_buf;
            }
        }

        private void EnsureCapacity(int expectRemaining)
        {
            int end = _position + expectRemaining;
            if (end  > _capacity)
            {
                int newCapacity = NormalizeCapacity(end);
                Expand(newCapacity);
            }
        }

    }
}