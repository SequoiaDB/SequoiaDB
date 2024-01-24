/*
 * Copyright 2011-2012 the original author or authors.
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

package org.springframework.data.sequoiadb.config;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;

import java.net.InetAddress;
import java.util.List;

import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.core.SequoiadbFactoryBean;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.sequoiadb.assist.CommandResult;
import org.springframework.data.sequoiadb.assist.ServerAddress;

@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class SdbNamespaceReplicaSetTests {

	@Autowired private ApplicationContext ctx;

	@Test
	@SuppressWarnings("unchecked")
	public void testParsingSequoiadbWithReplicaSets() throws Exception {

		assertTrue(ctx.containsBean("replicaSetSequoiadb"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&replicaSetSequoiadb");

		List<ServerAddress> replicaSetSeeds = (List<ServerAddress>) ReflectionTestUtils.getField(mfb, "replicaSetSeeds");

		assertThat(replicaSetSeeds, is(notNullValue()));
		assertThat(
				replicaSetSeeds,
				hasItems(new ServerAddress(InetAddress.getByName("127.0.0.1"), 10001),
						new ServerAddress(InetAddress.getByName("localhost"), 10002)));
	}

	@Test
	@SuppressWarnings("unchecked")
	public void testParsingWithPropertyPlaceHolder() throws Exception {

		assertTrue(ctx.containsBean("manyReplicaSetSequoiadb"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&manyReplicaSetSequoiadb");

		List<ServerAddress> replicaSetSeeds = (List<ServerAddress>) ReflectionTestUtils.getField(mfb, "replicaSetSeeds");

		assertThat(replicaSetSeeds, is(notNullValue()));
		assertThat(replicaSetSeeds, hasSize(3));
		assertThat(
				replicaSetSeeds,
				hasItems(new ServerAddress("192.168.174.130", 11810), new ServerAddress("192.168.174.130", 27018),
						new ServerAddress("192.168.174.130", 27019)));
	}

	@Test
	@Ignore("CI infrastructure does not yet support replica sets")
	public void testSequoiadbWithReplicaSets() {

		Sdb sdb = ctx.getBean(Sdb.class);
		assertEquals(2, sdb.getAllAddress().size());
		List<ServerAddress> servers = sdb.getAllAddress();
		assertEquals("127.0.0.1", servers.get(0).getHost());
		assertEquals("localhost", servers.get(1).getHost());
		assertEquals(10001, servers.get(0).getPort());
		assertEquals(10002, servers.get(1).getPort());

		SequoiadbTemplate template = new SequoiadbTemplate(sdb, "admin");
		CommandResult result = template.executeCommand("{replSetGetStatus : 1}");
		assertEquals("blort", result.getString("set"));
	}
}
