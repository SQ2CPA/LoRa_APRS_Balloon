#include "current_day.h"
#include <SPIFFS.h>

namespace Current_Day
{
    int read()
    {
        File file = SPIFFS.open("/day.txt", FILE_READ);

        if (!file)
        {
            return 0;
        }

        String data = file.readString();

        file.close();

        return data.toInt();
    }

    void write(int day)
    {
        File file = SPIFFS.open("/day.txt", FILE_WRITE);
        if (!file)
        {
            return;
        }

        file.print(String(day));
        file.close();
    }
}