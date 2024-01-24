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

import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.base.DBCollection;
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
		Sequoiadb sdb = new Sequoiadb(connString, "", "");
		try {
			DBCollection cl = Constants.getCL(sdb);

			// create match condition and modify rule
			BSONObject matcher = new BasicBSONObject();
			BSONObject modifier = new BasicBSONObject();
			BSONObject m = new BasicBSONObject();
			matcher.put("Id", 20);
			m.put("Age", 80);
			modifier.put("$set", m);

			UpdateResult result = cl.updateRecords(matcher, modifier);
			System.out.println(result);
		} catch (BaseException e) {
			e.printStackTrace();
		} finally {
			sdb.close();
		}
	}

}
