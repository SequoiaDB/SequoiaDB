/*
 * Copyright 2010-2011 the original author or authors.
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
package org.springframework.data.sequoiadb.core.mapping.event;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration;

@Configuration
public class ApplicationContextEventTestsAppConfig extends AbstractSequoiadbConfiguration {

	@Override
	public String getDatabaseName() {
		return "database";
	}

	@Override
	@Bean
	public Sdb sdb() throws Exception {
		return new SdbClient("127.0.0.1");
	}

	@Bean
	public PersonBeforeSaveListener personBeforeSaveListener() {
		return new PersonBeforeSaveListener();
	}

	@Bean
	public AfterSaveListener afterSaveListener() {
		return new AfterSaveListener();
	}

	@Bean
	public SimpleMappingEventListener simpleMappingEventListener() {
		return new SimpleMappingEventListener();
	}

}
