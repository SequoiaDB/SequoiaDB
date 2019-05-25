/*
 * Copyright 2011-2013 the original author or authors.
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

import org.junit.Before;
import org.junit.Test;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.beans.factory.config.ConstructorArgumentValues;
import org.springframework.beans.factory.config.ConstructorArgumentValues.ValueHolder;
import org.springframework.beans.factory.parsing.BeanDefinitionParsingException;
import org.springframework.beans.factory.support.BeanDefinitionReader;
import org.springframework.beans.factory.support.DefaultListableBeanFactory;
import org.springframework.beans.factory.xml.XmlBeanDefinitionReader;
import org.springframework.context.support.AbstractApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;
import org.springframework.core.io.ClassPathResource;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.SimpleSequoiadbFactory;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.sequoiadb.assist.SdbClient;

/**
 * Integration tests for {@link SequoiadbFactoryParser}.
 * 

 */
public class SequoiadbFactoryParserIntegrationTests {

	DefaultListableBeanFactory factory;
	BeanDefinitionReader reader;

	@Before
	public void setUp() {
		factory = new DefaultListableBeanFactory();
		reader = new XmlBeanDefinitionReader(factory);
	}

	@Test
	public void testWriteConcern() throws Exception {

		SimpleSequoiadbFactory dbFactory = new SimpleSequoiadbFactory(new SdbClient("localhost"), "database");
		dbFactory.setWriteConcern(WriteConcern.SAFE);
		dbFactory.getDb();

		assertThat(ReflectionTestUtils.getField(dbFactory, "writeConcern"), is((Object) WriteConcern.SAFE));
	}

	@Test
	public void parsesWriteConcern() {
		ClassPathXmlApplicationContext ctx = new ClassPathXmlApplicationContext("namespace/db-factory-bean.xml");
		assertWriteConcern(ctx, WriteConcern.SAFE);
	}

	@Test
	public void parsesCustomWriteConcern() {
		ClassPathXmlApplicationContext ctx = new ClassPathXmlApplicationContext(
				"namespace/db-factory-bean-custom-write-concern.xml");
		assertWriteConcern(ctx, new WriteConcern("rack1"));
	}

	/**
	 * @see DATA_JIRA-331
	 */
	@Test
	public void readsReplicasWriteConcernCorrectly() {

		AbstractApplicationContext ctx = new ClassPathXmlApplicationContext(
				"namespace/db-factory-bean-custom-write-concern.xml");
		SequoiadbFactory factory = ctx.getBean("second", SequoiadbFactory.class);
		DB db = factory.getDb();

		assertThat(db.getWriteConcern(), is(WriteConcern.REPLICAS_SAFE));
		ctx.close();
	}

	private void assertWriteConcern(ClassPathXmlApplicationContext ctx, WriteConcern expectedWriteConcern) {
		SimpleSequoiadbFactory dbFactory = ctx.getBean("first", SimpleSequoiadbFactory.class);
		DB db = dbFactory.getDb();
		assertThat(db.getName(), is("db"));

		WriteConcern configuredConcern = (WriteConcern) ReflectionTestUtils.getField(dbFactory, "writeConcern");

		MyWriteConcern myDbFactoryWriteConcern = new MyWriteConcern(configuredConcern);
		MyWriteConcern myDbWriteConcern = new MyWriteConcern(db.getWriteConcern());
		MyWriteConcern myExpectedWriteConcern = new MyWriteConcern(expectedWriteConcern);

		assertThat(myDbFactoryWriteConcern, is(myExpectedWriteConcern));
		assertThat(myDbWriteConcern, is(myExpectedWriteConcern));
		assertThat(myDbWriteConcern, is(myDbFactoryWriteConcern));
	}

	public void testWriteConcernEquality() {
		String s1 = new String("rack1");
		String s2 = new String("rack1");
		WriteConcern wc1 = new WriteConcern(s1);
		WriteConcern wc2 = new WriteConcern(s2);
		assertThat(wc1, is(wc2));
	}

	@Test
	public void createsDbFactoryBean() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/db-factory-bean.xml"));
		factory.getBean("first");
	}

	/**
	 * @see DATADOC-280
	 */
	@Test
	public void parsesMaxAutoConnectRetryTimeCorrectly() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/db-factory-bean.xml"));
		Sdb sdb = factory.getBean(Sdb.class);
		assertThat(sdb.getSequoiadbOptions().maxAutoConnectRetryTime, is(27L));
	}

	/**
	 * @see DATADOC-295
	 */
	@Test
	public void setsUpSequoiadbFactoryUsingASequoiadbUri() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/sdb-uri.xml"));
		BeanDefinition definition = factory.getBeanDefinition("sequoiadbFactory");
		ConstructorArgumentValues constructorArguments = definition.getConstructorArgumentValues();

		assertThat(constructorArguments.getArgumentCount(), is(1));
		ValueHolder argument = constructorArguments.getArgumentValue(0, SequoiadbURI.class);
		assertThat(argument, is(notNullValue()));
	}

	/**
	 * @see DATADOC-306
	 */
	@Test
	public void setsUpSequoiadbFactoryUsingAMongoUriWithoutCredentials() {

		reader.loadBeanDefinitions(new ClassPathResource("namespace/sdb-uri-no-credentials.xml"));
		BeanDefinition definition = factory.getBeanDefinition("sequoiadbFactory");
		ConstructorArgumentValues constructorArguments = definition.getConstructorArgumentValues();

		assertThat(constructorArguments.getArgumentCount(), is(1));
		ValueHolder argument = constructorArguments.getArgumentValue(0, SequoiadbURI.class);
		assertThat(argument, is(notNullValue()));

		SequoiadbFactory dbFactory = factory.getBean("sequoiadbFactory", SequoiadbFactory.class);
		DB db = dbFactory.getDb();
		assertThat(db.getName(), is("database"));
	}

	/**
	 * @see DATADOC-295
	 */
	@Test(expected = BeanDefinitionParsingException.class)
	public void rejectsUriPlusDetailedConfiguration() {
		reader.loadBeanDefinitions(new ClassPathResource("namespace/sdb-uri-and-details.xml"));
	}
}
