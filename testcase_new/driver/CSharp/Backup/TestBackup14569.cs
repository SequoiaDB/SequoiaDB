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
     * description:   backupOffline (BsonDocument options)；
     *                listBackup (BsonDocument options, BsonDocument matcher, BsonDocument selector, BsonDocument orderBy)
     *                removeBackup (BsonDocument options)
     *                1、执行backupOffline()备份指定组的数据库，其中设置备份名、指定组名、备份路径等参数信息 
     *                2、指定备份名查看数据库备份信息，检查备份信息是否正确,其中listBackup覆盖接口中所有参数 
     *                3、指定备份名删除数据库备份，查看删除结果              
     * testcase:    seqDB-14569 seqDB-13026
     * author:      chensiqin
     * date:        2019/04/11
     */

    [TestClass]
    public class TestBackup14569
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
        public void Test14569()
        {
            if (Common.IsStandalone(sdb) == true)
            {
                return;
            }
            List<string> dataGroups = Common.getDataGroupNames(sdb);

            string backupName1 = "backup14569_1";
            string path = SdbTestBase.workDir + "/%g";
            BsonDocument option1 = new BsonDocument();
            option1.Add("Name", backupName1);
            option1.Add("GroupName", new BsonArray(dataGroups));
            option1.Add("Path", path);

            string backupName2 = "backup14569_2";
            BsonDocument option2 = new BsonDocument();
            option2.Add("Name", backupName2);
            option2.Add("GroupName", new BsonArray(dataGroups));
            option2.Add("Path", path);

            //backup
            try {
                sdb.BackupOffline(option1);
                sdb.Backup(option2);
            } catch (BaseException e) {
                Assert.Fail("backup failed, errMsg:" + e.Message);
            }

            //list
            string groupName = dataGroups[0];
            string hostName = sdb.GetReplicaGroup(groupName).GetMaster().HostName;
            BsonDocument listOption = new BsonDocument();
            listOption.Add("Path", path);
            listOption.Add("HostName", hostName);
            listOption.Add("GroupName", groupName);

            BsonDocument matcher = new BsonDocument();
            matcher.Add("Name", backupName1);

            BsonDocument selector = new BsonDocument();
            selector.Add("Name", 1);
            selector.Add("NodeName", 1);
            selector.Add("GroupName", 1);
            selector.Add("StartTime", 1);

            BsonDocument orderBy = new BsonDocument();
            orderBy.Add("GroupName", -1);
            DBCursor cursor = null;
            try {
                cursor = sdb.ListBackup(listOption, matcher, selector, orderBy);
                while (cursor.Next() != null) 
                {
                    BsonDocument record = cursor.Current();
                    if (record.Contains("Name")) 
                    {
                        string actualBackupName = record.GetElement("Name").Value.ToString();

                        //check matcher
                        Assert.AreEqual(backupName1, actualBackupName);

                        //check selector
                        int keySetSize = record.ElementCount;
                        Assert.AreEqual(4, keySetSize);
                        Assert.IsTrue(record.Contains("Name"));
                        Assert.IsTrue(record.Contains("NodeName"));
                        Assert.IsTrue(record.Contains("GroupName"));
                        Assert.IsTrue(record.Contains("StartTime"));
                    }
                }
                cursor.Close();
            } catch (BaseException e) {
                Assert.Fail("list backup failed, errMsg:" + e.Message);
            }


            //remove backup
            BsonDocument removeOption = new BsonDocument();
            removeOption.Add("Name", backupName1);

            sdb.RemoveBackup(removeOption);

            cursor = sdb.ListBackup(removeOption, null, null, null);
            Assert.IsNull(cursor.Next());

        }

        [TestCleanup()]
        public void TearDown()
        {
            string path = SdbTestBase.workDir + "/%g";
            BsonDocument removeOption = new BsonDocument();
            removeOption.Add("Path", path);
            try
            {
                sdb.RemoveBackup(removeOption);
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -264)
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
