/* This is a modification of code produced Paul Sheer from 2038bug.com.
   His copyright, conditions and disclaimer are reproduced below.
*/

/* pivotal_gmtime_r - a replacement for gmtime/localtime/mktime
                      that works around the 2038 bug on 32-bit
                      systems. (Version 3)

   Copyright (C) 2005  Paul Sheer

   Redistribution and use in source form, with or without modification,
   is permitted provided that the above copyright notice, this list of
   conditions, the following disclaimer, and the following char array
   are retained.

   Redistribution and use in binary form must reproduce an
   acknowledgment: 'With software provided by http://2038bug.com/' in
   the documentation and/or other materials provided with the
   distribution, and wherever such acknowledgments are usually
   accessible in Your program.

   This software is provided "AS IS" and WITHOUT WARRANTY, either
   express or implied, including, without limitation, the warranties of
   NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE. THE ENTIRE RISK AS TO THE QUALITY OF THIS SOFTWARE IS WITH
   YOU. Under no circumstances and under no legal theory, whether in
   tort (including negligence), contract, or otherwise, shall the
   copyright owners be liable for any direct, indirect, special,
   incidental, or consequential damages of any character arising as a
   result of the use of this software including, without limitation,
   damages for loss of goodwill, work stoppage, computer failure or
   malfunction, or any and all other commercial damages or losses. This
   limitation of liability shall not apply to liability for death or
   personal injury resulting from copyright owners' negligence to the
   extent applicable law prohibits such limitation. Some jurisdictions
   do not allow the exclusion or limitation of incidental or
   consequential damages, so this exclusion and limitation may not apply
   to You.

*/

/*

Programmers who have available to them 64-bit time values as a 'long
long' type can use gmtime64_r() instead, which correctly converts the
time even on 32-bit systems. Whether you have 64-bit time values
will depend on the operating system.

Both functions are 64-bit clean and should work as expected on 64-bit
systems. They have not yet been tested on 64-bit systems however.

localtime64_r() is a 64-bit equivalent of localtime_r().

gmtime64_r() is a 64-bit equivalent of gmtime_r().

*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static const int julian_days_by_month[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

static const int length_of_year[2] = { 365, 366 };

#define LEAP_CHECK(n)	((!(((n) + 1900) % 400) || (!(((n) + 1900) % 4) && (((n) + 1900) % 100))) != 0)
#define WRAP(a,b,m)	((a) = ((a) <  0  ) ? ((b)--, (a) + (m)) : (a))


void _check_tm(struct tm *tm)
{
    /* Don't forget leap seconds */
    assert(tm->tm_sec  >= 0 && tm->tm_sec <= 61);
    assert(tm->tm_min  >= 0 && tm->tm_min <= 59);
    assert(tm->tm_hour >= 0 && tm->tm_hour <= 23);
    assert(tm->tm_mday >= 1 && tm->tm_mday <= 31);
    assert(tm->tm_mon  >= 0 && tm->tm_mon  <= 11);
    assert(tm->tm_wday >= 0 && tm->tm_wday <= 6);
    assert(tm->tm_yday >= 0 && tm->tm_yday <= 365);
    assert(   tm->tm_gmtoff >= -24 * 60 * 60
           && tm->tm_gmtoff <=  24 * 60 * 60);
}

int _cycle_offset(int year)
{
    int start_year = 2000;
    int year_diff  = year - start_year;
    int cycle_offset = 0;
    cycle_offset += year_diff / 100;
    cycle_offset -= year_diff / 400;
    
    return cycle_offset;
}

/* For a given year after 2038, pick the latest possible matching
   year in the 28 year calendar cycle.
   XXX Correct for 100/400 year boundries
*/
#define SOLAR_CYCLE_LENGTH 28
int _safe_year(int year)
{
    int safe_year = 2016;
    int year_cycle;
    
    year_cycle = (year + _cycle_offset(year)) % SOLAR_CYCLE_LENGTH;
    
    if( year_cycle <= 21 )
        safe_year += year_cycle;
    else
        safe_year =  safe_year - (SOLAR_CYCLE_LENGTH - year_cycle);

    assert(safe_year <= 2037 && safe_year >= 2009);
    
    /* printf("year: %d, safe_year: %d\n", year, safe_year); */

    return safe_year;
}

struct tm *gmtime64_r (const long long *in_time, struct tm *p)
{
    int v_tm_sec, v_tm_min, v_tm_hour, v_tm_mon, v_tm_wday, v_tm_tday;
    int leap;
    long m;
    long long time = *in_time;
    
    p->tm_gmtoff = 0;
    p->tm_isdst  = 0;
    p->tm_zone   = "UTC";
    
    v_tm_sec =  time % 60;
    time /= 60;
    v_tm_min =  time % 60;
    time /= 60;
    v_tm_hour = time % 24;
    time /= 24;
    v_tm_tday = time;
    WRAP (v_tm_sec, v_tm_min, 60);
    WRAP (v_tm_min, v_tm_hour, 60);
    WRAP (v_tm_hour, v_tm_tday, 24);
    if ((v_tm_wday = (v_tm_tday + 4) % 7) < 0)
        v_tm_wday += 7;
    m = (long) v_tm_tday;
    if (m >= 0) {
        p->tm_year = 70;
        leap = LEAP_CHECK (p->tm_year);
        while (m >= (long) length_of_year[leap]) {
            m -= (long) length_of_year[leap];
            p->tm_year++;
            leap = LEAP_CHECK (p->tm_year);
        }
        v_tm_mon = 0;
        while (m >= (long) days_in_month[leap][v_tm_mon]) {
            m -= (long) days_in_month[leap][v_tm_mon];
            v_tm_mon++;
        }
    } else {
        p->tm_year = 69;
        leap = LEAP_CHECK (p->tm_year);
        while (m < (long) -length_of_year[leap]) {
            m += (long) length_of_year[leap];
            p->tm_year--;
            leap = LEAP_CHECK (p->tm_year);
        }
        v_tm_mon = 11;
        while (m < (long) -days_in_month[leap][v_tm_mon]) {
            m += (long) days_in_month[leap][v_tm_mon];
            v_tm_mon--;
        }
        m += (long) days_in_month[leap][v_tm_mon];
    }
    p->tm_mday = (int) m + 1;
    p->tm_yday = julian_days_by_month[leap][v_tm_mon] + m;
    p->tm_sec = v_tm_sec, p->tm_min = v_tm_min, p->tm_hour = v_tm_hour,
        p->tm_mon = v_tm_mon, p->tm_wday = v_tm_wday;
    
    _check_tm(p);

    return p;
}


struct tm *localtime64_r (const long long *time, struct tm *local_tm)
{
    time_t safe_time;
    struct tm gm_tm;
    int orig_year;
    int month_diff;

    gmtime64_r(time, &gm_tm);
    orig_year = gm_tm.tm_year;

    if (gm_tm.tm_year > (2037 - 1900))
        gm_tm.tm_year = _safe_year(gm_tm.tm_year + 1900) - 1900;

    safe_time = timegm(&gm_tm);
    localtime_r(&safe_time, local_tm);

    local_tm->tm_year = orig_year;
    month_diff = local_tm->tm_mon - gm_tm.tm_mon;
    
    if( month_diff == 11 )
        local_tm->tm_year--;

    if( month_diff == -11 )
        local_tm->tm_year++;

    _check_tm(local_tm);
    
    return local_tm;
}
