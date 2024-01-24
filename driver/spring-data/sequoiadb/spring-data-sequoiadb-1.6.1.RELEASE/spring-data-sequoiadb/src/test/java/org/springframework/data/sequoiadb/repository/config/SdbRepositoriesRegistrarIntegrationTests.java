/*
 * Copyright 2012 the original author or authors.
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
package org.springframework.data.sequoiadb.repository.config;

import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.SimpleSequoiadbFactory;
import org.springframework.data.sequoiadb.repository.PersonRepository;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

/**
 * Integration tests for {@link SequoiadbRepositoriesRegistrar}.
 * 

 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration
public class SdbRepositoriesRegistrarIntegrationTests {

	@Configuration
	@EnableSequoiadbRepositories(basePackages = "org.springframework.data.sequoiadb.repository")
	static class Config {

		@Bean
		public SequoiadbOperations sequoiadbTemplate() throws Exception {
			return new SequoiadbTemplate(new SimpleSequoiadbFactory(new SdbClient(), "database"));
		}
	}

	@Autowired PersonRepository personRepository;
	@Autowired ApplicationContext context;

	@Test
	public void testConfiguration() {

	}

	/**
	 * @see DATA_JIRA-901
	 */
	@Test
	public void registersTypePredictingPostProcessor() {

		for (String name : context.getBeanDefinitionNames()) {
			if (name.startsWith("org.springframework.data.repository.core.support.RepositoryInterfaceAwareBeanPostProcessor")) {
				return;
			}
		}

		fail("Expected to find a bean with name starting with RepositoryInterfaceAwareBeanPostProcessor");
	}
}
