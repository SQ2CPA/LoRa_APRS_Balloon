#include "current_day.h"
#include <Preferences.h>
#include <RTClib.h>

extern Preferences preferences;
extern int currentDay;

bool isLoaded = false;
uint32_t launchedAt = 0;

namespace Current_Day
{
    void read()
    {
        launchedAt = preferences.getULong64("launch_date", 0);
        isLoaded = true;
    }

    void getDays(uint32_t date)
    {
        if (currentDay > -1)
            return;

        if (launchedAt == 0)
        {
            currentDay = 0;
            return;
        }

        DateTime d1 = DateTime(launchedAt % 100 + 2000, (launchedAt / 100) % 100, launchedAt / 10000);
        DateTime d2 = DateTime(date % 100 + 2000, (date / 100) % 100, date / 10000);

        TimeSpan diff = d1 - d2;

        currentDay = int(diff.days());
    }

    void setIfNotSet(uint32_t date)
    {
        if (isLoaded && launchedAt == 0)
        {
            preferences.putULong64("launch_date", date);
            launchedAt = date;
        }
    }
}