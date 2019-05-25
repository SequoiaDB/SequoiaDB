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

import com.sequoiadb.base.CollectionSpace;
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
		Sequoiadb sdb = null;
		CollectionSpace cs = null;
		DBCollection cl = null;

		try {
			sdb = new Sequoiadb(connString, "", "");
		} catch (BaseException e) {
			System.out.println("Failed to connect to database: " + connString
					+ ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}

		if (sdb.isCollectionSpaceExist(Constants.CS_NAME))
			cs = sdb.getCollectionSpace(Constants.CS_NAME);
		else
			cs = sdb.createCollectionSpace(Constants.CS_NAME);

		if (cs.isCollectionExist(Constants.CL_NAME))
			cl = cs.getCollection(Constants.CL_NAME);
		else
			cl = cs.createCollection(Constants.CL_NAME);

		// inserted data
		BSONObject insertor1 = Constants.createChineseRecord();
		BSONObject insertor2 = Constants.createEnglishRecord();
		List<BSONObject> list = Constants.createNameList(200);

		// insert operation
		try {
			// chinese record
			cl.insert(insertor1);
			// english record
			cl.insert(insertor2);
			// bulk insert
			cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
			System.out.println("Successfully insert records");
		} catch (BaseException e) {
			System.out.println("Failed to insert chinese record, ErrorType = "
					+ e.getErrorType());
		} catch (Exception e) {
			e.printStackTrace();
			sdb.disconnect();
			System.exit(1);
		}

		sdb.disconnect();
	}

}
