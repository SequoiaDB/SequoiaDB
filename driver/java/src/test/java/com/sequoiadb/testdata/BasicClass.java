package com.sequoiadb.testdata;

import java.util.Date;

public class BasicClass {
    private long id = 0;
    private String name = "name";
    private int age = 25;
    private double scode = 99.999;

    private Integer ageInteger = 25;
    private Long idLong = 0L;
    private Double scodeDouble = 9999.001;
    private Float floatvalue = 012121.212f;
    private Date date = new Date();



    public long getId() {
        return id;
    }

    public void setId(long value) {
        id = value;
    }

    public String getName() {
        return name;
    }

    public void setName(String value) {
        name = value;
    }

    public int getAge() {
        return age;
    }

    public void setAge(int value) {
        age = value;
    }

    public double getScode() {
        return scode;
    }

    public void setScode(double value) {
        scode = value;
    }

    public Integer getAgeInteger() {
        return ageInteger;
    }

    public void setAgeInteger(Integer value) {
        ageInteger = value;
    }

    public Long getIdLong() {
        return idLong;
    }

    public void setIdLong(Long value) {
        idLong = value;
    }

    public Double getScodeDouble() {
        return scodeDouble;
    }

    public void setScodeDouble(Double value) {
        scodeDouble = value;
    }

    /**
     * @return the floatvalue
     */
    public Float getFloatvalue() {
        return floatvalue;
    }

    /**
     * @param floatvalue the floatvalue to set
     */
    public void setFloatvalue(Float floatvalue) {
        this.floatvalue = floatvalue;
    }

    /**
     * @return the date
     */
    public Date getDate() {
        return date;
    }

    /**
     * @param date the date to set
     */
    public void setDate(Date date) {
        this.date = date;
    }

    @Override
    public boolean equals(Object other) {

        if (!(other instanceof BasicClass)) {
            return false;
        }
        BasicClass otherObj = (BasicClass) other;

        if (this.age != otherObj.age) {
            return false;
        }
        if (!this.ageInteger.equals(otherObj.ageInteger)) {
            return false;
        }
        if (!this.floatvalue.equals(otherObj.floatvalue)) {
            return false;
        }
        if (this.id != otherObj.id) {
            return false;
        }
        if (!this.idLong.equals(otherObj.idLong)) {
            return false;
        }
        if (!this.name.equals(otherObj.name)) {
            return false;
        }
        if (this.scode - otherObj.scode > 0.01) {
            return false;
        }
        if (!this.scodeDouble.equals(otherObj.scodeDouble)) {
            return false;
        }
        if (this.floatvalue - otherObj.floatvalue > 0.01) {
            return false;
        }
        if (!this.date.equals(otherObj.date)) {
            return false;
        }

        return true;
    }
}