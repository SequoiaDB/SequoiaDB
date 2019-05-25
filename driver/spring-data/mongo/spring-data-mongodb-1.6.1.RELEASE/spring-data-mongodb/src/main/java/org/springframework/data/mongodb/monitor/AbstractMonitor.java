/*
 * Copyright 2002-2012 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.mongodb.monitor;

import com.sequoiadb.exception.SDBError;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.mongodb.core.MongoDbUtils;

import org.springframework.data.mongodb.assist.CommandResult;
import org.springframework.data.mongodb.assist.DB;
import org.springframework.data.mongodb.assist.Mongo;
import org.springframework.data.mongodb.assist.MongoException;

/**
 * Base class to encapsulate common configuration settings when connecting to a database
 * 
 * @author Mark Pollack
 */
public abstract class AbstractMonitor {

	private final Logger logger = LoggerFactory.getLogger(getClass());

	protected Mongo mongo;
	private String username;
	private String password;

	/**
	 * Sets the username to use to connect to the Mongo database
	 * 
	 * @param username The username to use
	 */
	public void setUsername(String username) {
		this.username = username;
	}

	/**
	 * Sets the password to use to authenticate with the Mongo database.
	 * 
	 * @param password The password to use
	 */
	public void setPassword(String password) {
		this.password = password;
	}

	public CommandResult getServerStatus() {
		CommandResult result = getDb("admin").command("serverStatus");
		if (!result.ok()) {
			logger.error("Could not query for server status.  Command Result = " + result);
			throw new MongoException(SDBError.SDB_SYS.getErrorCode(),
					"could not query for server status.  Command Result = " + result);
		}
		return result;
	}

	public DB getDb(String databaseName) {
		return MongoDbUtils.getDB(mongo, databaseName, new UserCredentials(username, password));
	}
}
