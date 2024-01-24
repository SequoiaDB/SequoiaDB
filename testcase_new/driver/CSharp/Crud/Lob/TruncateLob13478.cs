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
     * description: test TruncateLob()
     *              interface: TruncateLob(ObjectId id, long length)
     * testcase:    13478, 13475
     * author:      linsuqiang
     * date:        2018/3/17
     */

    [TestClass]
    public class TruncateLobParamVerify13478
    {
        private Sequoiadb sdb = null;
        private CollectionSpace cs  = null;
        private DBCollection cl = null;
        private const string clName = "cl13478";
        private ObjectId oid;
        private int lobSize = 64;

        private const int SDB_FNE = -4;
        private const int SDB_INVALID_ARG = -6;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
            cs = sdb.GetCollectionSpace(SdbTestBase.csName);
            cl = cs.CreateCollection(clName);
            oid = LobUtils.CreateAndWriteLob(cl, new byte[lobSize]);
        }
        
        [TestMethod()]
        public void TestTruncateLobParam13478()
        {
            long normalVal = 10;
            long maxLong = long.MaxValue;
            long minLong = long.MinValue;
            ObjectId inexistOid = new ObjectId("5a114afdf980000000000000");

            // illegal parameter
            TryTuncateLob(cl, inexistOid, normalVal, SDB_FNE);
            TryTuncateLob(cl, oid, -1, SDB_INVALID_ARG);
            TryTuncateLob(cl, oid, minLong, SDB_INVALID_ARG);

            // legal parameter
            cl.TruncateLob(oid, maxLong);
            cl.TruncateLob(oid, lobSize / 2);

            // basic function
            long expLobSize = lobSize / 2;
            long actLobSize1 = GetSizeByListLob(cl, oid);
            long actLobSize2 = GetSizeByDBLob(cl, oid);
            Assert.AreEqual(expLobSize, actLobSize1);
            Assert.AreEqual(actLobSize1, actLobSize2);
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

        private void TryTuncateLob(DBCollection cl, ObjectId oid, long length, int ignoredCode)
        {
            try 
            {
                cl.TruncateLob(oid, length);
                Assert.Fail("TruncateLob(" + oid.ToString() + ", " + length + ") should not succeed");
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != ignoredCode)
                {
                    throw e;
                }
            }
        }

        private long GetSizeByListLob(DBCollection cl, ObjectId oid)
        {
            bool foundOid = false;
            long lobSize = -1;
            DBCursor cursor = cl.ListLobs();
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                ObjectId currOid = doc.GetElement("Oid").Value.AsObjectId;
                if (currOid.Equals(oid))
                {
                    lobSize = doc.GetElement("Size").Value.AsInt64;
                    foundOid = true;
                    break;
                }
            }
            cursor.Close();
            if (!foundOid)
                throw new ApplicationException("oid not found");
            return lobSize;
        }

        private long GetSizeByDBLob(DBCollection cl, ObjectId oid)
        {
            DBLob lob = cl.OpenLob(oid);
            long lobSize = lob.GetSize();
            lob.Close();
            return lobSize;
        }
    }
}
