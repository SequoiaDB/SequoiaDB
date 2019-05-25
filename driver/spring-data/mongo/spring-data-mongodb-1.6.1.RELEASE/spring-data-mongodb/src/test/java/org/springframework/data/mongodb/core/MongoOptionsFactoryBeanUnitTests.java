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
package org.springframework.data.mongodb.core;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import javax.net.ssl.SSLSocketFactory;

import org.junit.Test;

import org.springframework.data.mongodb.assist.MongoOptions;

/**
 * Unit tests for {@link MongoOptionsFactoryBean}.
 * 
 * @author Oliver Gierke
 * @author Mike Saavedra
 */
public class MongoOptionsFactoryBeanUnitTests {

	/**
	 * @see DATADOC-280
	 */
	@Test
	public void setsMaxConnectRetryTime() {

		MongoOptionsFactoryBean bean = new MongoOptionsFactoryBean();
		bean.afterPropertiesSet();

		MongoOptions options = bean.getObject();
	}

	/**
	 * @see DATAMONGO-764
	 */
	@Test
	public void testSslConnection() {

		MongoOptionsFactoryBean bean = new MongoOptionsFactoryBean();
		bean.afterPropertiesSet();

		MongoOptions options = bean.getObject();
	}
}
