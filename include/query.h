#ifndef QUERY_H
#define QUERY_H

#include <Arduino.h>

namespace Query
{
    bool process(const String &packet);
}

#endif