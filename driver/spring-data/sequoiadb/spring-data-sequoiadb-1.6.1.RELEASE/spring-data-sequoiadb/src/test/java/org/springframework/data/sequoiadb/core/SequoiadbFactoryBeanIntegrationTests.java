/*
 * Copyright 2012-2013 the original author or authors.
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
package org.springframework.data.sequoiadb.core;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.junit.Test;
import org.springframework.beans.factory.support.DefaultListableBeanFactory;
import org.springframework.beans.factory.support.RootBeanDefinition;
import org.springframework.data.sequoiadb.config.ServerAddressPropertyEditor;
import org.springframework.data.sequoiadb.config.WriteConcernPropertyEditor;
import org.springframework.test.util.ReflectionTestUtils;

import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.ServerAddress;
import org.springframework.data.sequoiadb.assist.WriteConcern;

/**
 * Integration tests for {@link SequoiadbFactoryBean}.
 * 


 */
public class SequoiadbFactoryBeanIntegrationTests {

	/**
	 * @see DATA_JIRA-408
	 */
	@Test
	public void convertsWriteConcernCorrectly() {

		RootBeanDefinition definition = new RootBeanDefinition(SequoiadbFactoryBean.class);
		definition.getPropertyValues().addPropertyValue("writeConcern", "SAFE");

		DefaultListableBeanFactory factory = new DefaultListableBeanFactory();
		factory.registerCustomEditor(WriteConcern.class, WriteConcernPropertyEditor.class);
		factory.registerBeanDefinition("factory", definition);

		SequoiadbFactoryBean bean = factory.getBean("&factory", SequoiadbFactoryBean.class);
		assertThat(ReflectionTestUtils.getField(bean, "writeConcern"), is((Object) WriteConcern.SAFE));
	}

	/**
	 * @see DATA_JIRA-693
	 */
	@Test
	public void createSequoiadbInstanceWithHostAndEmptyReplicaSets() {

		RootBeanDefinition definition = new RootBeanDefinition(SequoiadbFactoryBean.class);
		definition.getPropertyValues().addPropertyValue("host", "localhost");
		definition.getPropertyValues().addPropertyValue("replicaPair", "");

		DefaultListableBeanFactory factory = new DefaultListableBeanFactory();
		factory.registerCustomEditor(ServerAddress.class, ServerAddressPropertyEditor.class);
		factory.registerBeanDefinition("factory", definition);

		Sdb sdb = factory.getBean(Sdb.class);
		assertNotNull(sdb);
	}
}
