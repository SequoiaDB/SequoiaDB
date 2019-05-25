using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using SequoiaDB;
using SequoiaDB.Bson;

namespace DriverTest
{
    class Constants
    {
        public static char[] constant = {   
            '0','1','2','3','4','5','6','7','8','9',  
            'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',   
            'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'   
        };

        public static Boolean isClusterEnv(Sequoiadb sdb)
        {
            try
            {
                sdb.ListReplicaGroups();
            }
            catch (BaseException e)
            {
                int errcode = e.ErrorCode;
                if (new BaseException("SDB_RTN_COORD_ONLY").ErrorCode == errcode)
                    return false;
                else
                    throw e;
            }
            catch (System.Exception e)
            {
                Console.WriteLine(e.StackTrace);
                Environment.Exit(0);
            }
            return true;
        }

        public static string GenerateRandomNumber(int Length)
        {
            System.Text.StringBuilder newRandom = new System.Text.StringBuilder(62);
            Random rd = new Random();
            for (int i = 0; i < Length; i++)
            {
                newRandom.Append(constant[rd.Next(62)]);
            }
            return newRandom.ToString();
        }
    }

}
