package com.sequoiadb.testdata;

import java.util.Arrays;

public class Student {
    public int age;
    public String name;

    public void setAge(int age) {
        this.age = age;
    }

    public int getAge() {
        return this.age;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return this.name;
    }

    public Student(int age, String name) {
        super();
        this.age = age;
        this.name = name;
    }

    public Student() {

    }

    @Override
    public String toString() {
        return "Student [age=" + age + ", name=" + name + "]";
    }

    @Override
    public int hashCode() {
        Object[] objects = new Object[]{age, name};
        return Arrays.hashCode(objects);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Student) {
            if (this == obj) {
                return true;
            }

            Student other = (Student) obj;
            return this.age == other.age && this.name.equals(other.name);
        } else {
            return false;
        }
    }
}
