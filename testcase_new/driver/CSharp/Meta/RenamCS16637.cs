using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Meta
{
    /**
     * description:  seqDB-16637 renamCS
     * testcase:     16637
     * author:       chensiqin
     * date:         2018/11/20
    */
    [TestClass]
    public class RenamCS16637
    {
        private Sequoiadb sdb = null;
        private string localCSName = "cs16637";
        private string newCSName = "newcs16637";
        private string localCLName = "cl16637";

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod()]
        public void Test16637()
        {
            if (sdb.IsCollectionSpaceExist(localCSName))
            {
                sdb.DropCollectionSpace(localCSName);
            }
            if (sdb.IsCollectionSpaceExist(newCSName))
            {
                sdb.DropCollectionSpace(newCSName);
            }
            sdb.CreateCollectionSpace(localCSName);
            sdb.RenameCollectionSpace(localCSName, newCSName);
            Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl);
            db.Connect();
            CollectionSpace cs =  db.GetCollectionSpace(newCSName);
            cs.CreateCollection(localCLName);
            CheckSnapshotCS(db, localCSName, newCSName, 1);
            if (db.IsCollectionSpaceExist(localCSName))
            {
                db.DropCollectionSpace(localCSName);
            }
            if (db.IsCollectionSpaceExist(newCSName))
            {
                db.DropCollectionSpace(newCSName);
            }
            if (db != null)
            {
                db.Disconnect();
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            if (sdb != null)
            {
                sdb.Disconnect();
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }


        public void CheckSnapshotCS(Sequoiadb localDB, string oldCSName, string newCSName, int clNum)
        {
            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", oldCSName);
            DBCursor cur = localDB.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONSPACES, matcher, null, null);
            cur.Next();
            Assert.AreEqual(null, cur.Current());
            cur.Close();

            matcher = new BsonDocument();
            matcher.Add("Name", newCSName);
            cur = localDB.GetSnapshot(SDBConst.SDB_SNAP_COLLECTIONSPACES, matcher, null, null);
            cur.Next();
            BsonDocument doc = null;
            doc = cur.Current();
            Assert.AreNotEqual(null, doc);
            BsonArray arr = (BsonArray)doc.GetElement("Collection").Value;
            Assert.AreEqual(clNum, arr.Count());
            cur.Close();
        }
    }
}
