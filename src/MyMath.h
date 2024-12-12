#pragma once

static double MyMath_avg(double avg, double val)
{
    if (avg == 0)
        avg = val;
    return 0.96 * avg + 0.04 * val;
}

static double MyMath_vpd_leaf(double avarage_temp, double avarage_humidity)
{
    double svp = 0.6108 * exp((17.67 * avarage_temp) / (avarage_temp + 243.5));
    double avp = avarage_humidity / 100 * svp;
    return svp - avp;
}

static double MyMath_vpd_air(double avarage_temp, double avarage_humidity)
{
    double svp = 0.6108 * exp((17.67 * avarage_temp) / (avarage_temp + 243.5));
    return (1-avarage_humidity/100) * svp;
}