using System;
using System.IO;
using System.Collections.Generic;

namespace SequoiaDB
{
    class Helper
    {
        private static readonly Logger logger = new Logger("Helper");

        internal static void AlignByteBuffer(ByteBuffer buffer, int inc)
        {
            if (inc > buffer.Remaining()) {
                throw new BaseException((int)Errors.errors.SDB_SYS, "inc is more than the remaining in ByteBuffer");
            }
            for (int i = 0; i < inc; i++)
            {
                buffer.PushByte(0);
            }
        }

        internal static byte[] RoundToMultipleX(byte[] byteArray, int multipler)
        {
            if (multipler == 0)
                return byteArray;

            int inLength = byteArray.Length;
            int newLength = inLength % multipler == 0 ? inLength : inLength + multipler - inLength % multipler;
            byte[] newArray = new byte[newLength];
            byteArray.CopyTo(newArray, 0);
            return newArray;
        }

        internal static int RoundToMultipleXLength(int inLength, int multipler)
        {
            if (multipler == 0)
                return inLength;
            return inLength % multipler == 0 ? inLength : inLength + multipler 
                   - inLength % multipler;
        }

        internal static byte[] ConcatByteArray(List<byte[]> inByteArrayList)
        {
          if (inByteArrayList == null || inByteArrayList.Count == 0)
             return null;

            MemoryStream stream = new MemoryStream();
            BinaryWriter output = new BinaryWriter(stream);

            for (int i = 0; i < inByteArrayList.Count; i++) {
                output.Write(inByteArrayList[i]);
            }
            output.Close();
            stream.Close();
            return stream.ToArray();
       }

        internal static List<byte[]> SplitByteArray(byte[] inByteArray, int length)
        {
            if (inByteArray == null)
                return null;
            List<byte[]> rtnList = new List<byte[]>();
            if (length >= inByteArray.Length)
            {
                rtnList.Add(inByteArray);
                rtnList.Add(null);
                return rtnList;
            }

            ByteBuffer buf = new ByteBuffer(inByteArray);
            rtnList.Add(buf.PopByteArray(length));
            rtnList.Add(buf.PopByteArray(inByteArray.Length - length));

            return rtnList;

        }

        internal static int ByteToInt(byte[] byteArray, bool isBigEndian)
        {
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in byteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
            }
            if (byteArray == null || byteArray.Length != 4)
                throw new BaseException("SDB_INVALIDSIZE");

            ByteBuffer buf = new ByteBuffer(byteArray);
            if (isBigEndian)
                buf.IsBigEndian = true;
            return buf.PopInt();
        }

        internal static long ByteToLong(byte[] byteArray, bool isBigEndian)
        {
            if (logger.IsDebugEnabled)
            {
                StringWriter buff = new StringWriter();
                foreach (byte by in byteArray)
                {
                    buff.Write(string.Format("{0:X}", by));
                }
            }
            if (byteArray == null || byteArray.Length != 8)
                throw new BaseException("SDB_INVALIDSIZE");

            ByteBuffer buf = new ByteBuffer(byteArray);
            if (isBigEndian)
                buf.IsBigEndian = true;
            return buf.PopLong();
        }

    }
}
