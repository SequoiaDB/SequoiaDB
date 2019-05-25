package com.sequoiadb.samples;

/******************************************************************************
 *
 * Name: Update.java
 * Description: This program demonstrates how to use the Java Driver to update
 * 			 	the data in DB
 * 				Get more details in API document
 *
 * ****************************************************************************/

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Update {
	public static void main(String[] args) {
		if (args.length != 1) {
			if (args.length != 1) {
				System.out
						.println("Please give the database server address <IP:Port>");
				System.exit(0);
			}
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

		// create match condition and modify rule
		BSONObject matcher = new BasicBSONObject();
		BSONObject modifier = new BasicBSONObject();
		BSONObject m = new BasicBSONObject();

		matcher.put("Id", 20);
		m.put("Age", 80);
		modifier.put("$set", m);
		cl.upsert(matcher, modifier, null);
		// query the updated data from db
		DBCursor cursor = cursor = cl.query(matcher, null, null, null);
		try {
			if(cursor.hasNext())
				System.out.println(cursor.getNext());
		} finally {
			cursor.close();
		}
		sdb.disconnect();
	}

}
