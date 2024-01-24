package org.springframework.data.sequoiadb.core.mapping;

/**
 <jon@jbrisbin.com>
 */
public class PersonWithLongDBRef extends BasePerson {

	@DBRef private PersonPojoLongId personPojoLongId;

	public PersonWithLongDBRef(Integer ssn, String firstName, String lastName, PersonPojoLongId personPojoLongId) {
		super(ssn, firstName, lastName);
		this.personPojoLongId = personPojoLongId;
	}

	public PersonPojoLongId getPersonPojoLongId() {
		return personPojoLongId;
	}

	public void setPersonPojoLongId(PersonPojoLongId personPojoLongId) {
		this.personPojoLongId = personPojoLongId;
	}

}
