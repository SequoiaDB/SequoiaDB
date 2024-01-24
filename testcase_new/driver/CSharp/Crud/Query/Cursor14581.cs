using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SequoiaDB;
using CSharp.TestCommon;
using SequoiaDB.Bson;

namespace CSharp.Crud.Query
{
    /**
     * description: test cursor related function
     *              interface: DBCursor.Close()
     *                         Sequoiadb.CloseAllCursor()
     * testcase:    14581
     * author:      linsuqiang
     * date:        2018/3/12
     */

    [TestClass]
    public class Cursor14581
    {
        private Sequoiadb sdb = null;

        [TestInitialize()]
        public void SetUp()
        {
            Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd hh:mm:ss:fff") + " begin: " + this.GetType().ToString());
            sdb = new Sequoiadb(SdbTestBase.coordUrl);
            sdb.Connect();
        }
        
        [TestMethod()]
        public void TestCursor14581()
        {
            // repeatly close
            DBCursor cursor = sdb.GetList(SDBConst.SDB_LIST_SESSIONS);
            cursor.Close();
            Assert.IsTrue(IsCursorClosed(cursor));
            cursor.Close();

            // close all cursor
            cursor = sdb.GetList(SDBConst.SDB_LIST_SESSIONS);
            DBCursor cursor2 = sdb.GetList(SDBConst.SDB_LIST_SESSIONS);
            sdb.CloseAllCursors();
            Assert.IsTrue(IsCursorClosed(cursor));
            Assert.IsTrue(IsCursorClosed(cursor2));

            // reuse cursor object
            cursor = sdb.GetList(SDBConst.SDB_LIST_SESSIONS);
            Assert.AreNotEqual(null, cursor.Next());
            cursor.Close();
            Assert.IsTrue(IsCursorClosed(cursor));
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

        private bool IsCursorClosed(DBCursor cursor)
        {
            try
            {
                cursor.Next();
                return false;
            }
            catch (BaseException e)
            {
                if (e.ErrorCode != -31 // SDB_DMS_CONTEXT_IS_CLOSE
                    && e.ErrorCode != -36) // SDB_RTN_CONTEXT_NOTEXIST
                {
                    Console.WriteLine(e.ToString());
                    return false;
                }
            }
            return true;
        }
    }
}
