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
package org.springframework.data.sequoiadb.core.mapping;

import org.springframework.context.annotation.Bean;
import org.springframework.data.sequoiadb.assist.Sdb;
import org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration;
import org.springframework.data.sequoiadb.core.mapping.event.LoggingEventListener;

import org.springframework.data.sequoiadb.assist.SdbClient;

public class GeoIndexedAppConfig extends AbstractSequoiadbConfiguration {

	public static String GEO_DB = "database";
	public static String GEO_COLLECTION = "geolocation";

	@Override
	public String getDatabaseName() {
		return GEO_DB;
	}

	@Override
	@Bean
	public Sdb sdb() throws Exception {
		return new SdbClient("127.0.0.1");
	}

	@Override
	public String getMappingBasePackage() {
		return "org.springframework.data.sequoiadb.core.core.mapping";
	}

	@Bean
	public LoggingEventListener mappingEventsListener() {
		return new LoggingEventListener();
	}
}
