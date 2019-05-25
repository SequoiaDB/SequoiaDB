/*
 * Copyright 2011-2013 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core.mapping.event;

import static org.hamcrest.CoreMatchers.*;
import static org.junit.Assert.*;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.Test;
import org.springframework.context.support.AbstractApplicationContext;
import org.springframework.context.support.ClassPathXmlApplicationContext;


import org.springframework.data.sequoiadb.core.mapping.Account;
import org.springframework.data.sequoiadb.repository.Contact;
import org.springframework.data.sequoiadb.repository.Person;

/**
 * Unit tests for {@link AbstractSequoiadbEventListener}.
 * 


 */
public class AbstractSdbEventListenerUnitTests {

	@Test
	public void invokesCallbackForEventForPerson() {

		SequoiadbMappingEvent<Person> event = new BeforeConvertEvent<Person>(new Person("Dave", "Matthews"));
		SamplePersonEventListener listener = new SamplePersonEventListener();
		listener.onApplicationEvent(event);
		assertThat(listener.invokedOnBeforeConvert, is(true));
	}

	@Test
	public void dropsEventIfNotForCorrectDomainType() {

		AbstractApplicationContext context = new ClassPathXmlApplicationContext();
		context.refresh();

		SamplePersonEventListener listener = new SamplePersonEventListener();
		context.addApplicationListener(listener);

		context.publishEvent(new BeforeConvertEvent<Person>(new Person("Dave", "Matthews")));
		assertThat(listener.invokedOnBeforeConvert, is(true));

		listener.invokedOnBeforeConvert = false;
		context.publishEvent(new BeforeConvertEvent<String>("Test"));
		assertThat(listener.invokedOnBeforeConvert, is(false));

		context.close();
	}

	/**
	 * @see DATA_JIRA-289
	 */
	@Test
	public void afterLoadEffectGetsHandledCorrectly() {

		SamplePersonEventListener listener = new SamplePersonEventListener();
		listener.onApplicationEvent(new AfterLoadEvent<Person>(new BasicBSONObject(), Person.class));
		assertThat(listener.invokedOnAfterLoad, is(true));
	}

	/**
	 * @see DATA_JIRA-289
	 */
	@Test
	public void afterLoadEventGetsFilteredForDomainType() {

		SamplePersonEventListener personListener = new SamplePersonEventListener();
		SampleAccountEventListener accountListener = new SampleAccountEventListener();
		personListener.onApplicationEvent(new AfterLoadEvent<Person>(new BasicBSONObject(), Person.class));
		accountListener.onApplicationEvent(new AfterLoadEvent<Person>(new BasicBSONObject(), Person.class));

		assertThat(personListener.invokedOnAfterLoad, is(true));
		assertThat(accountListener.invokedOnAfterLoad, is(false));
	}

	/**
	 * @see DATA_JIRA-289
	 */
	@Test
	public void afterLoadEventGetsFilteredForDomainTypeWorksForSubtypes() {

		SamplePersonEventListener personListener = new SamplePersonEventListener();
		SampleContactEventListener contactListener = new SampleContactEventListener();
		personListener.onApplicationEvent(new AfterLoadEvent<Person>(new BasicBSONObject(), Person.class));
		contactListener.onApplicationEvent(new AfterLoadEvent<Person>(new BasicBSONObject(), Person.class));

		assertThat(personListener.invokedOnAfterLoad, is(true));
		assertThat(contactListener.invokedOnAfterLoad, is(true));
	}

	/**
	 * @see DATA_JIRA-289
	 */
	@Test
	public void afterLoadEventGetsFilteredForDomainTypeWorksForSubtypes2() {

		SamplePersonEventListener personListener = new SamplePersonEventListener();
		SampleContactEventListener contactListener = new SampleContactEventListener();
		personListener.onApplicationEvent(new AfterLoadEvent<Contact>(new BasicBSONObject(), Contact.class));
		contactListener.onApplicationEvent(new AfterLoadEvent<Contact>(new BasicBSONObject(), Contact.class));

		assertThat(personListener.invokedOnAfterLoad, is(false));
		assertThat(contactListener.invokedOnAfterLoad, is(true));
	}

	/**
	 * @see DATA_JIRA-333
	 */
	@Test
	@SuppressWarnings({ "rawtypes", "unchecked" })
	public void handlesUntypedImplementations() {

		UntypedEventListener listener = new UntypedEventListener();
		listener.onApplicationEvent(new SequoiadbMappingEvent(new Object(), new BasicBSONObject()));
	}

	/**
	 * @see DATA_JIRA-545
	 */
	@Test
	public void invokeContactCallbackForPersonEvent() {

		SequoiadbMappingEvent<BSONObject> event = new BeforeDeleteEvent<Person>(new BasicBSONObject(), Person.class);
		SampleContactEventListener listener = new SampleContactEventListener();
		listener.onApplicationEvent(event);

		assertThat(listener.invokedOnBeforeDelete, is(true));
	}

	/**
	 * @see DATA_JIRA-545
	 */
	@Test
	public void invokePersonCallbackForPersonEvent() {

		SequoiadbMappingEvent<BSONObject> event = new BeforeDeleteEvent<Person>(new BasicBSONObject(), Person.class);
		SamplePersonEventListener listener = new SamplePersonEventListener();
		listener.onApplicationEvent(event);

		assertThat(listener.invokedOnBeforeDelete, is(true));
	}

	/**
	 * @see DATA_JIRA-545
	 */
	@Test
	public void dontInvokePersonCallbackForAccountEvent() {

		SequoiadbMappingEvent<BSONObject> event = new BeforeDeleteEvent<Account>(new BasicBSONObject(), Account.class);
		SamplePersonEventListener listener = new SamplePersonEventListener();
		listener.onApplicationEvent(event);

		assertThat(listener.invokedOnBeforeDelete, is(false));
	}

	/**
	 * @see DATA_JIRA-545
	 */
	@Test
	public void donInvokePersonCallbackForUntypedEvent() {

		SequoiadbMappingEvent<BSONObject> event = new BeforeDeleteEvent<Account>(new BasicBSONObject(), null);
		SamplePersonEventListener listener = new SamplePersonEventListener();
		listener.onApplicationEvent(event);

		assertThat(listener.invokedOnBeforeDelete, is(false));
	}

	class SamplePersonEventListener extends AbstractSequoiadbEventListener<Person> {

		boolean invokedOnBeforeConvert;
		boolean invokedOnAfterLoad;
		boolean invokedOnBeforeDelete;
		boolean invokedOnAfterDelete;

		@Override
		public void onBeforeConvert(Person source) {
			invokedOnBeforeConvert = true;
		}

		@Override
		public void onAfterLoad(BSONObject dbo) {
			invokedOnAfterLoad = true;
		}

		@Override
		public void onAfterDelete(BSONObject dbo) {
			invokedOnAfterDelete = true;
		}

		@Override
		public void onBeforeDelete(BSONObject dbo) {
			invokedOnBeforeDelete = true;
		}
	}

	class SampleContactEventListener extends AbstractSequoiadbEventListener<Contact> {

		boolean invokedOnBeforeConvert;
		boolean invokedOnAfterLoad;
		boolean invokedOnBeforeDelete;
		boolean invokedOnAfterDelete;

		@Override
		public void onBeforeConvert(Contact source) {
			invokedOnBeforeConvert = true;
		}

		@Override
		public void onAfterLoad(BSONObject dbo) {
			invokedOnAfterLoad = true;
		}

		@Override
		public void onAfterDelete(BSONObject dbo) {
			invokedOnAfterDelete = true;
		}

		@Override
		public void onBeforeDelete(BSONObject dbo) {
			invokedOnBeforeDelete = true;
		}

	}

	class SampleAccountEventListener extends AbstractSequoiadbEventListener<Account> {

		boolean invokedOnBeforeConvert;
		boolean invokedOnAfterLoad;

		@Override
		public void onBeforeConvert(Account source) {
			invokedOnBeforeConvert = true;
		}

		@Override
		public void onAfterLoad(BSONObject dbo) {
			invokedOnAfterLoad = true;
		}
	}

	@SuppressWarnings("rawtypes")
	class UntypedEventListener extends AbstractSequoiadbEventListener {

	}
}
