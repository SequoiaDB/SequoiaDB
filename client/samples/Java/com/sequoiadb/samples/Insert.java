package com.sequoiadb.samples;

/******************************************************************************
 *
 * Name: Insert.java
 * Description: This program demonstrates how to use the Java Driver to
 *				insert data into DB
 * 				Get more details in API document
 *
 * ****************************************************************************/

import java.util.List;

import org.bson.BSONObject;

import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;


public class Insert {

	public static void main(String[] args) {
        if (args.length != 1) {
            System.out
                    .println("Please give the database server address <IP:Port>");
            System.exit(1);
        }

		// the database server address
		String connString = args[0];
		Sequoiadb sdb = new Sequoiadb(connString, "", "");
		try {
			DBCollection cl = Constants.getCL(sdb);

			// inserted data
			BSONObject doc1 = Constants.createChineseRecord();
			BSONObject doc2 = Constants.createEnglishRecord();
			List<BSONObject> list = Constants.createNameList(200);

			// chinese record
			InsertResult result1 = cl.insertRecord(doc1);
			System.out.println(result1);
			// english record
			InsertResult result2 = cl.insertRecord(doc2);
			System.out.println(result2);
			// bulk insert
			InsertResult result3 = cl.bulkInsert(list, new InsertOption().setFlag( InsertOption.FLG_INSERT_CONTONDUP ) );
			System.out.println(result3);
		} catch (BaseException e) {
			e.printStackTrace();
		} finally {
			sdb.close();
		}
	}
}
