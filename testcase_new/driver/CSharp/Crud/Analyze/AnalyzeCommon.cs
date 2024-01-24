using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using SequoiaDB.Bson;
using SequoiaDB;
using CSharp.TestCommon;

namespace CSharp.Crud.Analyze
{
    public class AnalyzeCommon
    {
        public static List<string> GetDataGroupNames(Sequoiadb sdb)
        {
            List<string> list = new List<string>();
            BsonDocument matcher = new BsonDocument();
            BsonDocument selector = new BsonDocument();
            BsonDocument orderBy = new BsonDocument();
            matcher.Add("Role", 0);
            selector.Add("GroupName", "");
            DBCursor cursor = sdb.GetList(7, matcher, selector, null);
            while (cursor.Next() != null)
            {
                BsonDocument doc = cursor.Current();
                BsonElement element = doc.GetElement("GroupName");
                list.Add(element.Value.ToString());
            }
            cursor.Close();
            return list;
        }

        public static bool IsStandalone(Sequoiadb sdb)
        {
            bool flag = false;
            try
            {
                sdb.ListReplicaGroups();
            }
            catch (BaseException e)
            {
                if (e.ErrorCode == -159)
                {
                    flag = true;
                }
            }
            return flag;
        }

        public static void InsertDataWithIndex(DBCollection cl)
        {
            List<BsonDocument> recs = new List<BsonDocument>();
            const int recNum = 2000;
            for (int i = 0; i < recNum; ++i)
            {
                recs.Add(new BsonDocument("a", 0));
            }
            Random rand = new Random();
            for (int i = 0; i < recNum; ++i)
            {
                recs.Add(new BsonDocument("a", rand.Next(10000)));
            }
            cl.BulkInsert(recs, 0);
            cl.CreateIndex("aIndex", new BsonDocument("a", 1), false, false);
        }

        public static void CheckScanTypeByExplain(DBCollection cl, string expScanType)
        {
            BsonDocument cond = new BsonDocument("a", 0);
            BsonDocument options = new BsonDocument("Run", true);
            DBCursor cursor = cl.Explain(cond, null, null, null, 0, -1, 0, options);
            string actScanType = cursor.Next().GetValue("ScanType").ToString();
            cursor.Close();
            if (expScanType != actScanType)
            {
                throw new Exception("wrong scan type. expect: " + expScanType + ", actual: " + actScanType);
            }
        }
    }
}
