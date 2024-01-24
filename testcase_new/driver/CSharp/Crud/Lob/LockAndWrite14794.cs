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
     * description: lock and write repeatly
     * testcase:    14794
     * author:      linsuqiang
     * date:        2018/3/17
     */

    [TestClass]
    public class LockAndWrite14794
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs  = null;
        private DBCollection cl = null;
        private const string clName = "cl14794";
        private ObjectId oid;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            oid = LobUtils.CreateAndWriteLob(cl, new byte[0]);
        }
        
        [TestMethod()]
        public void TestLockAndWrite14794()
        {
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_WRITE);
            const int partSize = 127;
            byte[] partA = LobUtils.GetRandomBytes(partSize);
            byte[] partB = LobUtils.GetRandomBytes(partSize);
            byte[] partC = LobUtils.GetRandomBytes(partSize);

            lob.Lock(0, partSize);
            lob.Write(partA);
            lob.Lock(2 * partSize, partSize);
            lob.Seek(partSize, DBLob.SDB_LOB_SEEK_CUR);
            lob.Write(partC);
            lob.Lock(partSize, partSize);
            lob.Seek(partSize, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(partB);
            lob.Close();

            byte[] expData = new byte[0];
            expData = LobUtils.AppendBuff(expData, partA, 0);
            expData = LobUtils.AppendBuff(expData, partB, partSize);
            expData = LobUtils.AppendBuff(expData, partC, 2 * partSize);
            byte[] actData = ReadLob(cl, oid);
            LobUtils.AssertByteArrayEqual(expData, actData);
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

        private byte[] ReadLob(DBCollection cl, ObjectId oid)
        {
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);
            byte[] readBuf = new byte[(int)lob.GetSize()];
            long readLen = lob.Read(readBuf);
            Assert.AreEqual(readLen, lob.GetSize());
            lob.Close();
            return readBuf;
        }
    }
}
