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
package org.springframework.data.sequoiadb.repository.support;

import static org.hamcrest.Matchers.*;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.SequoiadbOperations;
import org.springframework.data.sequoiadb.core.convert.SequoiadbConverter;
import org.springframework.data.sequoiadb.repository.ContactRepository;
import org.springframework.data.repository.core.support.RepositoryFactorySupport;
import org.springframework.test.util.ReflectionTestUtils;

/**
 * Unit tests for {@link SequoiadbRepositoryFactoryBean}.
 * 

 */
@RunWith(MockitoJUnitRunner.class)
public class SequoiadbRepositoryFactoryBeanUnitTests {

	@Mock
    SequoiadbOperations operations;
	@Mock
    SequoiadbConverter converter;
	@Mock @SuppressWarnings("rawtypes") MappingContext context;

	@Test
	@SuppressWarnings("rawtypes")
	public void addsIndexEnsuringQueryCreationListenerIfConfigured() {

		SequoiadbRepositoryFactoryBean factory = new SequoiadbRepositoryFactoryBean();
		factory.setCreateIndexesForQueryMethods(true);

		List<Object> listeners = getListenersFromFactory(factory);
		assertThat(listeners.isEmpty(), is(false));
		assertThat(listeners, hasItem(instanceOf(IndexEnsuringQueryCreationListener.class)));
	}

	@Test
	@SuppressWarnings("rawtypes")
	public void doesNotAddIndexEnsuringQueryCreationListenerByDefault() {

		List<Object> listeners = getListenersFromFactory(new SequoiadbRepositoryFactoryBean());
		assertThat(listeners.size(), is(1));
	}

	@SuppressWarnings({ "unchecked", "rawtypes" })
	private List<Object> getListenersFromFactory(SequoiadbRepositoryFactoryBean factoryBean) {

		when(operations.getConverter()).thenReturn(converter);
		when(converter.getMappingContext()).thenReturn(context);

		factoryBean.setLazyInit(true);
		factoryBean.setSequoiadbOperations(operations);
		factoryBean.setRepositoryInterface(ContactRepository.class);
		factoryBean.afterPropertiesSet();

		RepositoryFactorySupport factory = factoryBean.createRepositoryFactory();
		return (List<Object>) ReflectionTestUtils.getField(factory, "queryPostProcessors");
	}
}
