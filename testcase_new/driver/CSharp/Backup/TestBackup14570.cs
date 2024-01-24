using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Backup
{
    /**
     * description:  listBackup (BsonDocument options, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)             
     * testcase:    14570
     * author:      chensiqin
     * date:        2019/04/11
     */

    [TestClass]
    public class TestBackup14570
    {
        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }

        [TestMethod]
        public void Test14570()
        {
            if (Common.IsStandalone(sdb) == true)
            {
                return;
            }
            List<string> dataGroups = Common.getDataGroupNames(sdb);
            //set backup configure
            String backupName = "backup14570";
            BsonDocument option = new BsonDocument();
            option.Add("Name", backupName);

            //backup
            try
            {
                sdb.BackupOffline(option);
            }
            catch (BaseException e)
            {
                Assert.Fail("backup failed, errMsg:" + e.Message);
            }

            //list
            try
            {
                DBCursor cursor = sdb.ListBackup(null, null, null, null);
                while (cursor.Next() != null)
                {
                    BsonDocument record = cursor.Current();
                    if (record.Contains("Name"))
                    {
                        string actualBackupName = record.GetElement("Name").Value.ToString();

                        //check
                        Assert.AreEqual(backupName, actualBackupName);
                    }
                }
                cursor.Close();
            }
            catch (BaseException e)
            {
                Assert.Fail("list backup failed, errMsg:" + e.Message);
            }


            //remove backup
            try
            {
                sdb.RemoveBackup(null);
                //check
                DBCursor cursor = sdb.ListBackup(null, null, null, null);
                Assert.IsNull(cursor.Next());
            }
            catch (BaseException e)
            {
                Assert.Fail("remove backup failed, errMsg:" + e.Message);
            }
            finally
            {
                sdb.RemoveBackup(null);
            }
        }

        [TestCleanup()]
        public void TearDown()
        {
            try
            {
                sdb.RemoveBackup(null);
            }
            catch (BaseException e)
            {
                Assert.Fail("clear env failed, errMsg:" + e.Message);
            }
            finally
            {
                if (sdb != null)
                {
                    sdb.Disconnect();
                }
            }
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " end  : " + this.GetType().ToString());
        }
    }
}
