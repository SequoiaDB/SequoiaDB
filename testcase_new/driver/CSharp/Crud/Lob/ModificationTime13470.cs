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
     * description: test modification time
     *              interface: GetCreateTime()
     *                         GetModificationTime()
     * testcase:    13470
     * author:      linsuqiang
     * date:        2018/3/17
     */

    [TestClass]
    public class ModificationTime13470
    {
        private Sequoiadb sdb       = null;
        private CollectionSpace cs  = null;
        private DBCollection cl     = null;
        private const string clName = "cl13470";

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
        public void TestModificationTime13470()
        {
            // create lob
            DBLob lob = cl.CreateLob(); 
            lob.Close();
            long createTime = lob.GetCreateTime();
            long initModTime = lob.GetModificationTime();
            Assert.IsTrue((createTime < initModTime) || (createTime == initModTime));

            // modify lob
            ObjectId id = lob.GetID();
            DBLob wLob = cl.OpenLob(id, DBLob.SDB_LOB_WRITE);
            const int lobSize = 16;
            wLob.Write(new byte[lobSize]);
            wLob.Close();
            long writeModTime = wLob.GetModificationTime();
            Assert.IsTrue(initModTime < writeModTime);

            // not modify lob
            DBLob rLob = cl.OpenLob(id, DBLob.SDB_LOB_READ);
            byte[] readBuf = new byte[lobSize];
            int readLen = rLob.Read(readBuf);
            Assert.AreEqual(lobSize, readLen);
            rLob.Close();
            long readModTime = rLob.GetModificationTime();
            Assert.IsTrue(writeModTime == readModTime);
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
    }
}
