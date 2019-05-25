/*
 * Copyright 2002-2013 the original author or authors.
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

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.net.UnknownHostException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.Mongo;

/**
 * This test class assumes that you are already running the MongoDB server.
 * 
 * @author Mark Pollack
 * @author Thomas Darimont
 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration("classpath:infrastructure.xml")
public class MongoMonitorIntegrationTests {

	@Autowired Mongo mongo;

	@Test
	public void serverInfo() {
		ServerInfo serverInfo = new ServerInfo(mongo);
		serverInfo.getVersion();
		Assert.isTrue(StringUtils.hasText("1."));
	}

	/**
	 * @throws UnknownHostException
	 * @see DATAMONGO-685
	 */
	@Test
	public void getHostNameShouldReturnServerNameReportedByMongo() throws UnknownHostException {

		ServerInfo serverInfo = new ServerInfo(mongo);

		String hostName = null;
		try {
			hostName = serverInfo.getHostName();
		} catch (UnknownHostException e) {
			throw e;
		}

		assertThat(hostName, is(notNullValue()));
		assertThat(hostName, is("127.0.0.1"));
	}

	@Test
	public void operationCounters() {
		OperationCounters operationCounters = new OperationCounters(mongo);
		operationCounters.getInsertCount();
	}
}
