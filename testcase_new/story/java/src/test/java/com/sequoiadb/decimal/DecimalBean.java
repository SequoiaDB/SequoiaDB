package com.sequoiadb.decimal;

import java.math.BigDecimal;

import org.bson.types.BSONDecimal;

public class DecimalBean {
    private BSONDecimal decimalNoPrecision;
    private BSONDecimal decimalWithPrecision;
    private BigDecimal bigDecimal;

    public BSONDecimal getDecimalNoPrecision() {
        return decimalNoPrecision;
    }

    public void setDecimalNoPrecision( BSONDecimal decimalNoPrecision ) {
        this.decimalNoPrecision = decimalNoPrecision;
    }

    public BSONDecimal getDecimalWithPrecision() {
        return decimalWithPrecision;
    }

    public void setDecimalWithPrecision( BSONDecimal decimalWithPrecision ) {
        this.decimalWithPrecision = decimalWithPrecision;
    }

    public BigDecimal getBigDecimal() {
        return bigDecimal;
    }

    public void setBigDecimal( BigDecimal bigDecimal ) {
        this.bigDecimal = bigDecimal;
    }

    @Override
    public String toString() {
        return "DecimalBean [decimalNoPrecision=" + decimalNoPrecision
                + ", decimalWithPrecision=" + decimalWithPrecision
                + ", bigDecimal=" + bigDecimal + "]";
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result
                + ( ( bigDecimal == null ) ? 0 : bigDecimal.hashCode() );
        result = prime * result + ( ( decimalNoPrecision == null ) ? 0
                : decimalNoPrecision.hashCode() );
        result = prime * result + ( ( decimalWithPrecision == null ) ? 0
                : decimalWithPrecision.hashCode() );
        return result;
    }

    @Override
    public boolean equals( Object obj ) {
        if ( this == obj )
            return true;
        if ( obj == null )
            return false;
        if ( getClass() != obj.getClass() )
            return false;
        DecimalBean other = ( DecimalBean ) obj;
        if ( bigDecimal == null ) {
            if ( other.bigDecimal != null )
                return false;
        } else if ( !bigDecimal.equals( other.bigDecimal ) )
            return false;
        if ( decimalNoPrecision == null ) {
            if ( other.decimalNoPrecision != null )
                return false;
        } else if ( !decimalNoPrecision.equals( other.decimalNoPrecision ) )
            return false;
        if ( decimalWithPrecision == null ) {
            if ( other.decimalWithPrecision != null )
                return false;
        } else if ( !decimalWithPrecision.equals( other.decimalWithPrecision ) )
            return false;
        return true;
    }

}
