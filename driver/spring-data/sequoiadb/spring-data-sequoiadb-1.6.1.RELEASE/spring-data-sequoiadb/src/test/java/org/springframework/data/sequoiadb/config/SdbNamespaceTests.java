/*
 * Copyright 2010-2014 the original author or authors.
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

import static org.junit.Assert.*;
import static org.springframework.test.util.ReflectionTestUtils.*;

import javax.net.ssl.SSLSocketFactory;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.data.authentication.UserCredentials;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.SequoiadbOptions;
import org.springframework.data.sequoiadb.core.SequoiadbFactoryBean;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.gridfs.GridFsOperations;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.WriteConcern;

/**
 * Integration tests for the SequoiaDB namespace.
 *
 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class SdbNamespaceTests {

	@Autowired ApplicationContext ctx;

	@Test
	public void testSequoiadbSingleton() throws Exception {

		assertTrue(ctx.containsBean("noAttrSequoiadb"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&noAttrSequoiadb");

		assertNull(getField(mfb, "host"));
		assertNull(getField(mfb, "port"));
	}

	@Test
	public void testSequoiadbSingletonWithAttributes() throws Exception {

		assertTrue(ctx.containsBean("defaultSequoiadb"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&defaultSequoiadb");

		String host = (String) getField(mfb, "host");
		Integer port = (Integer) getField(mfb, "port");

		assertEquals("localhost", host);
		assertEquals(new Integer(11810), port);

		SequoiadbOptions options = (SequoiadbOptions) getField(mfb, "sequoiadbOptions");
		assertFalse("By default socketFactory should not be a SSLSocketFactory",
				options.getSocketFactory() instanceof SSLSocketFactory);
	}

	/**
	 * @see DATA_JIRA-764
	 */
	@Test
	public void testSequoiadbSingletonWithSslEnabled() throws Exception {

		assertTrue(ctx.containsBean("sequoiadbSsl"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&sequoiadbSsl");

		SequoiadbOptions options = (SequoiadbOptions) getField(mfb, "sequoiadbOptions");
		assertTrue("socketFactory should be a SSLSocketFactory", options.getSocketFactory() instanceof SSLSocketFactory);
	}

	/**
	 * @see DATA_JIRA-764
	 */
	@Test
	public void testSequoiadbSingletonWithSslEnabledAndCustomSslSocketFactory() throws Exception {

		assertTrue(ctx.containsBean("sequoiadbSslWithCustomSslFactory"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&sequoiadbSslWithCustomSslFactory");

		SSLSocketFactory customSslSocketFactory = ctx.getBean("customSslSocketFactory", SSLSocketFactory.class);
		SequoiadbOptions options = (SequoiadbOptions) getField(mfb, "sequoiadbOptions");

		assertTrue("socketFactory should be a SSLSocketFactory", options.getSocketFactory() instanceof SSLSocketFactory);
		assertSame(customSslSocketFactory, options.getSocketFactory());
	}

	@Test
	public void testSecondSequoiadbFactory() {

		assertTrue(ctx.containsBean("secondSequoiadbFactory"));
		SequoiadbFactory dbf = (SequoiadbFactory) ctx.getBean("secondSequoiadbFactory");

		Sdb sdb = (Sdb) getField(dbf, "sdb");
		assertEquals("localhost", sdb.getAddress().getHost());
		assertEquals(11810, sdb.getAddress().getPort());
		assertEquals(new UserCredentials("joe", "secret"), getField(dbf, "credentials"));
		assertEquals("database", getField(dbf, "databaseName"));
	}

	/**
	 * @see DATA_JIRA-789
	 */
	@Test
	public void testThirdSequoiadbFactory() {

		assertTrue(ctx.containsBean("thirdSequoiadbFactory"));

		SequoiadbFactory dbf = (SequoiadbFactory) ctx.getBean("thirdSequoiadbFactory");
		Sdb sdb = (Sdb) getField(dbf, "sdb");

		assertEquals("localhost", sdb.getAddress().getHost());
		assertEquals(11810, sdb.getAddress().getPort());
		assertEquals(new UserCredentials("joe", "secret"), getField(dbf, "credentials"));
		assertEquals("database", getField(dbf, "databaseName"));
		assertEquals("admin", getField(dbf, "authenticationDatabaseName"));
	}

	/**
	 * @see DATA_JIRA-140
	 */
	@Test
	public void testSequoiadbTemplateFactory() {

		assertTrue(ctx.containsBean("sequoiadbTemplate"));
		SequoiadbOperations operations = (SequoiadbOperations) ctx.getBean("sequoiadbTemplate");

		SequoiadbFactory dbf = (SequoiadbFactory) getField(operations, "sequoiadbFactory");
		assertEquals("database", getField(dbf, "databaseName"));

		SequoiadbConverter converter = (SequoiadbConverter) getField(operations, "sequoiadbConverter");
		assertNotNull(converter);
	}

	/**
	 * @see DATA_JIRA-140
	 */
	@Test
	public void testSecondSequoiadbTemplateFactory() {

		assertTrue(ctx.containsBean("anotherSequoiadbTemplate"));
		SequoiadbOperations operations = (SequoiadbOperations) ctx.getBean("anotherSequoiadbTemplate");

		SequoiadbFactory dbf = (SequoiadbFactory) getField(operations, "sequoiadbFactory");
		assertEquals("database", getField(dbf, "databaseName"));

		WriteConcern writeConcern = (WriteConcern) getField(operations, "writeConcern");
		assertEquals(WriteConcern.SAFE, writeConcern);
	}

	/**
	 * @see DATA_JIRA-628
	 */
	@Test
	public void testGridFsTemplateFactory() {

		assertTrue(ctx.containsBean("gridFsTemplate"));
		GridFsOperations operations = (GridFsOperations) ctx.getBean("gridFsTemplate");

		SequoiadbFactory dbf = (SequoiadbFactory) getField(operations, "dbFactory");
		assertEquals("database", getField(dbf, "databaseName"));

		SequoiadbConverter converter = (SequoiadbConverter) getField(operations, "converter");
		assertNotNull(converter);
	}

	/**
	 * @see DATA_JIRA-628
	 */
	@Test
	public void testSecondGridFsTemplateFactory() {

		assertTrue(ctx.containsBean("secondGridFsTemplate"));
		GridFsOperations operations = (GridFsOperations) ctx.getBean("secondGridFsTemplate");

		SequoiadbFactory dbf = (SequoiadbFactory) getField(operations, "dbFactory");
		assertEquals("database", getField(dbf, "databaseName"));
		assertEquals(null, getField(operations, "bucket"));

		SequoiadbConverter converter = (SequoiadbConverter) getField(operations, "converter");
		assertNotNull(converter);
	}

	/**
	 * @see DATA_JIRA-823
	 */
	@Test
	public void testThirdGridFsTemplateFactory() {

		assertTrue(ctx.containsBean("thirdGridFsTemplate"));
		GridFsOperations operations = (GridFsOperations) ctx.getBean("thirdGridFsTemplate");

		SequoiadbFactory dbf = (SequoiadbFactory) getField(operations, "dbFactory");
		assertEquals("database", getField(dbf, "databaseName"));
		assertEquals("bucketString", getField(operations, "bucket"));

		SequoiadbConverter converter = (SequoiadbConverter) getField(operations, "converter");
		assertNotNull(converter);
	}

	@Test
	@SuppressWarnings("deprecation")
	public void testSequoiadbSingletonWithPropertyPlaceHolders() throws Exception {

		assertTrue(ctx.containsBean("sdb"));
		SequoiadbFactoryBean mfb = (SequoiadbFactoryBean) ctx.getBean("&sdb");

		String host = (String) getField(mfb, "host");
		Integer port = (Integer) getField(mfb, "port");

		assertEquals("127.0.0.1", host);
		assertEquals(new Integer(11810), port);

		Sdb sdb = mfb.getObject();
		SequoiadbOptions sdbOpts = sdb.getSequoiadbOptions();

		assertEquals(8, sdbOpts.connectionsPerHost);
		assertEquals(1000, sdbOpts.connectTimeout);
		assertEquals(1500, sdbOpts.maxWaitTime);
		assertEquals(true, sdbOpts.autoConnectRetry);
		assertEquals(1500, sdbOpts.socketTimeout);
		assertEquals(4, sdbOpts.threadsAllowedToBlockForConnectionMultiplier);
		assertEquals(true, sdbOpts.socketKeepAlive);
		assertEquals(true, sdbOpts.fsync);
		assertEquals(true, sdbOpts.slaveOk);
		assertEquals(1, sdbOpts.getWriteConcern().getW());
		assertEquals(0, sdbOpts.getWriteConcern().getWtimeout());
		assertEquals(true, sdbOpts.getWriteConcern().fsync());
	}
}
