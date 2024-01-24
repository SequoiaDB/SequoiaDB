using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Lob
{
    /**
     * description: LockAndSeek() parameter verification
     *              interface: LockAndSeek (long offset, long length)
     * testcase:    13477
     * author:      linsuqiang
     * date:        2018/3/17
     */

    [TestClass]
    public class LockAndSeekParamVerify13477
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs  = null;
        private DBCollection cl = null;
        private const string clName = "cl13477";
        private ObjectId oid;
        private const int SDB_INVALID_ARG = -6;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            oid = LobUtils.CreateAndWriteLob(cl, new byte[64]);
        }
        
        [TestMethod()]
        public void TestLockAndSeek13477()
        {
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_WRITE);
            long normalVal = 10;
            long maxLong = long.MaxValue;
            long minLong = long.MinValue;

            // illegal parameter
            TryLockAndSeek(lob, -1, normalVal, SDB_INVALID_ARG);
            TryLockAndSeek(lob, minLong, normalVal, SDB_INVALID_ARG);
            TryLockAndSeek(lob, normalVal, -2, SDB_INVALID_ARG);
            TryLockAndSeek(lob, normalVal, 0, SDB_INVALID_ARG);
            TryLockAndSeek(lob, normalVal, minLong, SDB_INVALID_ARG);
            TryLockAndSeek(lob, maxLong / 2 + 1, maxLong / 2 + 1, SDB_INVALID_ARG); // offset + length > maxLong

            // legal parameter
            lob.LockAndSeek(maxLong - 1, 1);
            lob.LockAndSeek(normalVal, -1);
            lob.LockAndSeek(0, maxLong);

            // check legal lock usable
            lob.Seek(0, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(new byte[32]);
            lob.Close();
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                cs.DropCollection(clName);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();    
                }
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
            }
        }

        private void TryLockAndSeek(DBLob lob, long offset, long length, int ignoredCode)
        {
            try 
            {
                lob.LockAndSeek(offset, length);
                Assert.Fail("LockAndSeek(" + offset + ", " + length + ") should not succeed");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != ignoredCode)
                {
                    throw e;
                }
            }
        }
    }
}
