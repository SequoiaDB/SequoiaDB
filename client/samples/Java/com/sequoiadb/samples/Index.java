package com.sequoiadb.samples;

/******************************************************************************
 * 
 * Name: Index.java 
 * Description: This program demonstrates how to use the Java Driver to
 * 				operate on index in DB
 *				Get more details in API document
 * 
 ******************************************************************************/

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Index {
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

		// create index key, index on attribute 'Id' by ASC(1)/DESC(-1)
		BSONObject key = new BasicBSONObject();
		key.put("Id", 1);
		boolean isUnique = true;
		boolean enforced = true;
		try {
			// you can refer to the API to know more about this method
			cl.createIndex(Constants.INDEX_NAME, key, isUnique, enforced);
			// or
			// cl.createIndex(Constants.INDEX_NAME, "{'Id':1}", isUnique, enforced);

			// drop index
			cl.dropIndex(Constants.INDEX_NAME);
		} catch (BaseException e) {
			e.printStackTrace();
		}

		sdb.disconnect();
	}

}
