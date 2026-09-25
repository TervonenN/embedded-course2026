#include <string.h>
#include "TimeParser.h"
#include <ctype.h>

// time format: HHMMSS (6 characters)
int time_parse(char *time) {
    if (time == nullptr) {
        return TIME_ARRAY_ERROR;
    }

    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;
    }

    for (int index = 0; index < 6; index++) {
        if (!isdigit((unsigned char)time[index])) {
            return TIME_ARRAY_ERROR;
        }
    }

    int hours = (time[0] - '0') * 10 + (time[1] - '0');
    int minutes = (time[2] - '0') * 10 + (time[3] - '0');
    int seconds = (time[4] - '0') * 10 + (time[5] - '0');

    if (hours > 23 || minutes > 59 || seconds > 59) {
        return TIME_VALUE_ERROR;
    }

    return hours * 3600 + minutes * 60 + seconds;
}
