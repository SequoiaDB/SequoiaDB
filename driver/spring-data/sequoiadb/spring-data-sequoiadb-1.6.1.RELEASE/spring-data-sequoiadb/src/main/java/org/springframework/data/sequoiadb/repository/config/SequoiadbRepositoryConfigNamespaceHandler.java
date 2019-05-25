/*
 * Copyright 2014 the original author or authors.
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

import org.springframework.beans.factory.xml.NamespaceHandler;
import org.springframework.data.sequoiadb.config.SequoiadbNamespaceHandler;
import org.springframework.data.repository.config.RepositoryBeanDefinitionParser;
import org.springframework.data.repository.config.RepositoryConfigurationExtension;

/**
 * {@link NamespaceHandler} to register repository configuration.
 * 

 */
public class SequoiadbRepositoryConfigNamespaceHandler extends SequoiadbNamespaceHandler {

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.config.SequoiadbNamespaceHandler#init()
	 */
	@Override
	public void init() {

		RepositoryConfigurationExtension extension = new SequoiadbRepositoryConfigurationExtension();
		RepositoryBeanDefinitionParser repositoryBeanDefinitionParser = new RepositoryBeanDefinitionParser(extension);

		registerBeanDefinitionParser("repositories", repositoryBeanDefinitionParser);

		super.init();
	}
}
