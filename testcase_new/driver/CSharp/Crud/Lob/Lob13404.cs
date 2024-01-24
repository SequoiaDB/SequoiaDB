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
    * description:   lock to write lob
    *                  test interface:  test the interface lock() and lockAndSeek()
    * testcase:      13404 
    * author:        wuyan
    * date:          2018/3/16
    */
    [TestClass]
    public class Lob13404
    {
        private const string clName = "writeLob13404";
        private Sequoiadb sdb = null;
        private CollectionSpace cs = null;
        private DBCollection cl = null;
        byte[] testLobBuff = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
        }

        [TestMethod()]
        public void TestLob13404()
        {          
            int writeLobSize = 1024 * 100;
            testLobBuff = LobUtils.GetRandomBytes(writeLobSize);
            ObjectId oid = LobUtils.CreateAndWriteLob(cl, testLobBuff);

            //lock and write lob
            int writeLobSize1 = 1024 * 50;
            int offset = 1024 * 20;
            byte[] writeBuff = LobUtils.GetRandomBytes(writeLobSize1);
            LockAndWriteLob(oid, writeBuff, offset);

            //lockAndSeek and write lob
            int writeLobSize2 = 1024 * 40;
            int rewriteoffset = 1024 * 80;
            byte[] rewriteBuff = LobUtils.GetRandomBytes(writeLobSize2);
            LockAndWriteLob(oid, rewriteBuff, rewriteoffset);

            //check the result
            ReadAndcheckResult(oid);
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

        private void LockAndWriteLob(ObjectId oid, byte[] writeBuff, int offset)
        {
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_WRITE);
            lob.Lock(offset, writeBuff.Length);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(writeBuff);
            lob.Close();
            testLobBuff = LobUtils.AppendBuff(testLobBuff, writeBuff, offset);
        }

        private void LockAndseekToRewriteLob(ObjectId oid, byte[] writeBuff, int offset)
        {
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_WRITE);
            lob.LockAndSeek(offset, writeBuff.Length);
            lob.Seek(offset, DBLob.SDB_LOB_SEEK_SET);
            lob.Write(writeBuff);
            testLobBuff = LobUtils.AppendBuff(testLobBuff, writeBuff, offset);
        }

        private void ReadAndcheckResult(ObjectId oid)
        {
            //check the all write lob     
            DBLob lob = cl.OpenLob(oid, DBLob.SDB_LOB_READ);     
            byte[] readLobBuff = new byte[(int)lob.GetSize()];
            lob.Read(readLobBuff);
            lob.Close();
            LobUtils.AssertByteArrayEqual(readLobBuff, testLobBuff);
        }
    }
}
