using System;
using System.Text;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /**
     * description:   public call function for test lob        
     * author:        wuyan
     * date:          2018/3/15
     */

    public class LobUtils
    {
        /**
	     * generating byte to write lob
	     *
	     * @param length generating byte stream size
	     * @return byte[] bytes
	     */
        public static byte[] GetRandomBytes(int length)
        {
            byte[] bytes = new byte[length];
            Random random = new Random();
            random.NextBytes(bytes);
            return bytes;
        }

        public static byte[] AppendBuff(byte[] testLobBuff, byte[] rewriteBuff, int offset)
        {
            byte[] appendBuff = null;
            if (testLobBuff.Length >= offset + rewriteBuff.Length)
            {
                appendBuff = new byte[testLobBuff.Length];
            }
            else
            {
                appendBuff = new byte[offset + rewriteBuff.Length];
            }

            Array.Copy(testLobBuff, 0, appendBuff, 0, testLobBuff.Length);
            Array.Copy(rewriteBuff, 0, appendBuff, offset, rewriteBuff.Length);
            return appendBuff;
        }

        public static void AssertByteArrayEqual(byte[] actual, byte[] expect)
        {
            AssertByteArrayEqual(actual, expect, "");
        }

        public static void AssertByteArrayEqual(byte[] actual, byte[] expect, string msg)
        {
            if (!CompareByteArray(actual, expect))
            {
                Assert.Fail("\nexpect: " + System.Text.Encoding.Default.GetString(expect)
                    + "\nbut actual: " + System.Text.Encoding.Default.GetString(actual) + "\n" + msg + "\n");   
            }
        }

        /**
	     * compare byte array
	     * @param byte array1,byte array2
	     * @return false/true
	     */
        public static bool CompareByteArray(byte[] array1, byte[] array2)
        {
            if (array1.Length != array2.Length) return false;
            if (array1 == null || array2 == null) return false;
            for (int i = 0; i < array1.Length; i++ )
            {
                if (array1[i] != array2[i])
                    return false;
            }
            return true;
        }

        public static ObjectId CreateAndWriteLob(DBCollection dbcl,  byte[] data)
        {
            DBLob lob = dbcl.CreateLob();
            lob.Write(data);
            ObjectId oid = lob.GetID();
            lob.Close();
            return oid;
        }

        public static void CreateAndWriteLob(DBCollection dbcl, ObjectId id, byte[] data)
        {
            DBLob lob = dbcl.CreateLob(id);
            lob.Write(data);
            lob.Close();
        }   

    }
}
