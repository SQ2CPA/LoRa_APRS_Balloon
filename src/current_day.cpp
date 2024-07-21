#include "current_day.h"
#include <Preferences.h>

extern Preferences preferences;

namespace Current_Day
{
    int read()
    {
        return preferences.getInt("current_day", 0);
    }

    void write(int day)
    {
        preferences.putInt("current_day", day);
    }
}