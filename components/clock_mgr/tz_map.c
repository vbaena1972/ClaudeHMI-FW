#include "tz_map.h"
#include <string.h>
#include <stdlib.h>

const tz_map_t TZ_MAP[] = {
    { "America/Bogota",        "COT5" },
    { "America/Lima",          "PET5" },
    { "America/Guayaquil",     "ECT5" },
    { "America/Panama",        "EST5" },
    { "America/Jamaica",       "EST5" },

    { "America/New_York",      "EST5EDT,M3.2.0,M11.1.0" },
    { "America/Chicago",       "CST6CDT,M3.2.0,M11.1.0" },
    { "America/Denver",        "MST7MDT,M3.2.0,M11.1.0" },
    { "America/Los_Angeles",   "PST8PDT,M3.2.0,M11.1.0" },

    { "America/Mexico_City",   "CST6CDT,M4.1.0,M10.5.0" },
    { "America/Monterrey",     "CST6CDT,M4.1.0,M10.5.0" },

    { "America/Santiago",      "CLT4CLST,M9.1.6/24,M4.1.6/24" },
    { "America/Asuncion",      "PYT4PYST,M10.1.0/0,M3.4.0/0" },
    { "America/Caracas",       "VET4" },

    { "America/Buenos_Aires",  "ART3" },
    { "America/Montevideo",    "UYT3UYST,M10.1.0,M3.2.0" },

    { "Europe/Madrid",         "CET-1CEST,M3.5.0,M10.5.0" },
    { "Europe/Paris",          "CET-1CEST,M3.5.0,M10.5.0" },
    { "Europe/Berlin",         "CET-1CEST,M3.5.0,M10.5.0" },

    { "Asia/Dubai",            "GST-4" },
    { "Asia/Riyadh",           "AST-3" },
    { "Asia/Kolkata",          "IST-5:30" },
    { "Asia/Shanghai",         "CST-8" },
    { "Asia/Tokyo",            "JST-9" },
    { "Asia/Seoul",            "KST-9" },

    { "Australia/Sydney",      "AEST-10AEDT,M10.1.0,M4.1.0" },
    { "Australia/Perth",       "AWST-8" },

    { "UTC",                   "UTC0" },
};
