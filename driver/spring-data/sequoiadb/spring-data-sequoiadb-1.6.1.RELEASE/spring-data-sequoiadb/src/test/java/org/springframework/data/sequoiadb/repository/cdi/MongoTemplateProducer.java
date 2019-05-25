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
package org.springframework.data.sequoiadb.repository.cdi;

import java.net.UnknownHostException;

import javax.enterprise.context.ApplicationScoped;
import javax.enterprise.inject.Produces;

import com.sequoiadb.exception.BaseException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.SequoiadbTemplate;
import org.springframework.data.sequoiadb.core.SimpleSequoiadbFactory;


/**
 * Simple component exposing a {@link SequoiadbOperations} instance as CDI bean.
 * 

 */
class SequoiadbTemplateProducer {

	@Produces
	@ApplicationScoped
	public SequoiadbOperations createSequoiadbTemplate() throws UnknownHostException, BaseException {

		SequoiadbFactory factory = new SimpleSequoiadbFactory(new SdbClient(), "database");
		return new SequoiadbTemplate(factory);
	}
}
