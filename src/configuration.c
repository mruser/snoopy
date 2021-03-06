/*
 * SNOOPY LOGGER
 *
 * File: configuration.c
 *
 * Copyright (c) 2014 bostjan@a2o.si
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



/*
 * Include all required C resources
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>



/*
 * Include all snoopy-related resources
 */
#include "snoopy.h"
#include "configuration.h"



/*
 * Include iniparser-related resources
 */
#include "lib/iniparser/src/iniparser.h"



/*
 * Storage of snoopy configuration, with default values
 */
snoopy_configuration_type   snoopy_configuration = {
    .initialized             = SNOOPY_TRUE,

    .config_file_enabled     = SNOOPY_FALSE,
    .config_file_path        = "",
    .config_file_parsed      = SNOOPY_FALSE,

#ifdef SNOOPY_ERROR_LOGGING_ENABLED
    .error_logging_enabled   = SNOOPY_TRUE,
#else
    .error_logging_enabled   = SNOOPY_FALSE,
#endif

    .message_format          = SNOOPY_LOG_MESSAGE_FORMAT,
    .message_format_malloced = SNOOPY_FALSE,

#ifdef SNOOPY_FILTER_ENABLED
    .filter_enabled          = SNOOPY_TRUE,
#else
    .filter_enabled          = SNOOPY_FALSE,
#endif
    .filter_chain            = SNOOPY_FILTER_CHAIN,
    .filter_chain_malloced   = SNOOPY_FALSE,

    .output_provider         = SNOOPY_OUTPUT_PROVIDER,
    .output_path             = SNOOPY_OUTPUT_PATH,

    .syslog_facility         = SNOOPY_CONF_SYSLOG_FACILITY,
    .syslog_level            = SNOOPY_CONF_SYSLOG_LEVEL,
};



/*
 * snoopy_configuration_ctor
 *
 * Description:
 *     Populates snoopy_configuration config variable storage with
 *     correct values, either from configuration file (if enabled)
 *     or from ./configure arguments, or defaults are used as last
 *     case scenario.
 *
 * Params:
 *     (none)
 *
 * Return:
 *     void
 */
void snoopy_configuration_ctor ()
{
    /* Parse INI file if enabled */
#ifdef SNOOPY_CONFIG_FILE
    snoopy_configuration_load_file(SNOOPY_CONFIG_FILE);
#endif
}



/*
 * snoopy_configuration_dtor
 *
 * Description:
 *     Frees all configuration-related malloced resources.
 *
 * Params:
 *     (none)
 *
 * Return:
 *     void
 */
void snoopy_configuration_dtor ()
{
    if (SNOOPY_TRUE == snoopy_configuration.message_format_malloced) {
        free(snoopy_configuration.message_format);
    }
    if (SNOOPY_TRUE == snoopy_configuration.filter_chain_malloced) {
        free(snoopy_configuration.filter_chain);
    }
}



/*
 * snoopy_configuration_load_file
 *
 * Description:
 *     Parses INI configuration file and overrides snoopy
 *     configuration with changed values.
 *
 * Params:
 *     file   Path log INI configuration file
 *
 * Return:
 *     int    0 on success, -1 on error openinf file, other int for other errors
 */
int snoopy_configuration_load_file (
    char *iniFilePath
) {
    dictionary *ini ;
    char       *confValString;   // Temporary query result space
    int         confValInt;      // Temporary query result space

    /* Tell snoopy we are using configuration file */
    snoopy_configuration.config_file_enabled = SNOOPY_TRUE;
    snoopy_configuration.config_file_path    = iniFilePath;

    /* Parse the INI configuration file first */
    ini = iniparser_load(iniFilePath);
    if (NULL == ini) {
        // TODO snoopy error handling
        return -1;
    }


    /* Pick out snoopy configuration variables */
    confValInt = iniparser_getboolean(ini, "snoopy:error_logging", -1);
    if (-1 != confValInt) {
        snoopy_configuration.error_logging_enabled = confValInt;
    }

    confValString = iniparser_getstring(ini, "snoopy:message_format", NULL);
    if (NULL != confValString) {
        snoopy_configuration.message_format          = strdup(confValString);
        snoopy_configuration.message_format_malloced = SNOOPY_TRUE;
    }

    confValString = iniparser_getstring(ini, "snoopy:filter_chain", NULL);
    if (NULL != confValString) {
        snoopy_configuration.filter_chain          = strdup(confValString);
        snoopy_configuration.filter_chain_malloced = SNOOPY_TRUE;
    }

    confValString = iniparser_getstring(ini, "snoopy:output", NULL);
    if (NULL != confValString) {
        snoopy_configuration_parse_output(confValString);
    }

    confValString = iniparser_getstring(ini, "snoopy:syslog_facility", NULL);
    if (NULL != confValString) {
        snoopy_configuration_parse_syslog_facility(confValString);
    }

    confValString = iniparser_getstring(ini, "snoopy:syslog_level", NULL);
    if (NULL != confValString) {
        snoopy_configuration_parse_syslog_level(confValString);
    }


    /* Housekeeping */
    snoopy_configuration.config_file_parsed = SNOOPY_TRUE;   // We have sucessfully parsed configuration file
    iniparser_freedict(ini);
    return 0;
}



/*
 * snoopy_configuration_parse_output
 *
 * Description:
 *     Parses configuration setting syslog_output and
 *     sets appropriate internal configuration variable(s).
 *     Uses default setting if unknown value.
 *
 * Params:
 *     confVal   Value from configuration file
 *
 * Return:
 *     void
 */
void snoopy_configuration_parse_output (
    char *confVal
) {
    if (strcmp(confVal, SNOOPY_OUTPUT_PROVIDER_DEVLOG) == 0) { snoopy_configuration.output_provider = SNOOPY_OUTPUT_PROVIDER_DEVLOG; }
    if (strcmp(confVal, SNOOPY_OUTPUT_PROVIDER_SYSLOG) == 0) { snoopy_configuration.output_provider = SNOOPY_OUTPUT_PROVIDER_SYSLOG; }

    // TODO other input providers
    // Clone string
    // Check if colon character is present, split into left and right string
    // If right string is non-empty, malloc it, and store path in there
    // TODO configure output at ./configue time
}



/*
 * snoopy_configuration_parse_syslog_facility
 *
 * Description:
 *     Parses configuration setting syslog_facility and
 *     sets appropriate config variable.
 *     Uses default setting if unknown value.
 *
 * Params:
 *     confVal   Value from configuration file
 *
 * Return:
 *     void
 */
void snoopy_configuration_parse_syslog_facility (
    char *confVal
) {
    char *confValCleaned;

    // First cleanup the value
    confValCleaned = snoopy_configuration_syslog_value_cleanup(confVal);

    // Evaluate and set configuration flag
    if      (strcmp(confValCleaned, "AUTH")     == 0) { snoopy_configuration.syslog_facility = LOG_AUTH;     }
    else if (strcmp(confValCleaned, "AUTHPRIV") == 0) { snoopy_configuration.syslog_facility = LOG_AUTHPRIV; }
    else if (strcmp(confValCleaned, "CRON")     == 0) { snoopy_configuration.syslog_facility = LOG_CRON;     }
    else if (strcmp(confValCleaned, "DAEMON")   == 0) { snoopy_configuration.syslog_facility = LOG_DAEMON;   }
    else if (strcmp(confValCleaned, "FTP")      == 0) { snoopy_configuration.syslog_facility = LOG_FTP;      }
    else if (strcmp(confValCleaned, "KERN")     == 0) { snoopy_configuration.syslog_facility = LOG_KERN;     }
    else if (strcmp(confValCleaned, "LOCAL0")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL0;   }
    else if (strcmp(confValCleaned, "LOCAL1")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL1;   }
    else if (strcmp(confValCleaned, "LOCAL2")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL2;   }
    else if (strcmp(confValCleaned, "LOCAL3")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL3;   }
    else if (strcmp(confValCleaned, "LOCAL4")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL4;   }
    else if (strcmp(confValCleaned, "LOCAL5")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL5;   }
    else if (strcmp(confValCleaned, "LOCAL6")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL6;   }
    else if (strcmp(confValCleaned, "LOCAL7")   == 0) { snoopy_configuration.syslog_facility = LOG_LOCAL7;   }
    else if (strcmp(confValCleaned, "LPR")      == 0) { snoopy_configuration.syslog_facility = LOG_LPR;      }
    else if (strcmp(confValCleaned, "MAIL")     == 0) { snoopy_configuration.syslog_facility = LOG_MAIL;     }
    else if (strcmp(confValCleaned, "NEWS")     == 0) { snoopy_configuration.syslog_facility = LOG_NEWS;     }
    else if (strcmp(confValCleaned, "SYSLOG")   == 0) { snoopy_configuration.syslog_facility = LOG_SYSLOG;   }
    else if (strcmp(confValCleaned, "USER")     == 0) { snoopy_configuration.syslog_facility = LOG_USER;     }
    else if (strcmp(confValCleaned, "UUCP")     == 0) { snoopy_configuration.syslog_facility = LOG_UUCP;     }
    else {
        snoopy_configuration.syslog_facility = SNOOPY_SYSLOG_FACILITY;
    }
}



/*
 * snoopy_configuration_parse_syslog_level
 *
 * Description:
 *     Parses configuration setting syslog_level and
 *     sets appropriate config variable.
 *     Uses default setting if unknown value.
 *
 * Params:
 *     confVal   Value from configuration file
 *
 * Return:
 *     void
 */
void snoopy_configuration_parse_syslog_level (
    char *confVal
) {
    char *confValCleaned;

    // First cleanup the value
    confValCleaned = snoopy_configuration_syslog_value_cleanup(confVal);

    // Evaluate and set configuration flag
    if      (strcmp(confValCleaned, "EMERG")   == 0) { snoopy_configuration.syslog_level = LOG_EMERG;   }
    else if (strcmp(confValCleaned, "ALERT")   == 0) { snoopy_configuration.syslog_level = LOG_ALERT;   }
    else if (strcmp(confValCleaned, "CRIT")    == 0) { snoopy_configuration.syslog_level = LOG_CRIT;    }
    else if (strcmp(confValCleaned, "ERR")     == 0) { snoopy_configuration.syslog_level = LOG_ERR;     }
    else if (strcmp(confValCleaned, "WARNING") == 0) { snoopy_configuration.syslog_level = LOG_WARNING; }
    else if (strcmp(confValCleaned, "NOTICE")  == 0) { snoopy_configuration.syslog_level = LOG_NOTICE;  }
    else if (strcmp(confValCleaned, "INFO")    == 0) { snoopy_configuration.syslog_level = LOG_INFO;    }
    else if (strcmp(confValCleaned, "DEBUG")   == 0) { snoopy_configuration.syslog_level = LOG_DEBUG;   }
    else {
        snoopy_configuration.syslog_level = SNOOPY_SYSLOG_LEVEL;
    }
}



/*
 * snoopy_configuration_syslog_value_cleanup
 *
 * Description:
 *     Convert existing string to upper case, and remove LOG_ prefix
 *
 * Params:
 *     confVal   Pointer to string to change and to be operated on
 *
 * Return:
 *     char *    Pointer to cleaned string (either the same as input
 *               or 4 characters advanced, to remove LOG_ prefix
 */
char *snoopy_configuration_syslog_value_cleanup (char *confVal)
{
    char *confValCleaned;

    // Initialize - just in case
    confValCleaned = confVal;

    // Convert to upper case
    snoopy_configuration_strtoupper(confVal);

    // Remove LOG_ prefix
    confValCleaned = snoopy_configuration_syslog_value_remove_prefix(confVal);

    return confValCleaned;
}



/*
 * snoopy_configuration_syslog_value_remove_prefix
 *
 * Description:
 *     Remove the LOG_ prefix, return pointer to new string (either equal
 *     or +4 chars advanced)
 *
 * Params:
 *     string   Pointer to string to remove LOG_ prefix
 *
 * Return:
 *     char *   Pointer to non LOG_ part of the string
 */
char *snoopy_configuration_syslog_value_remove_prefix (char *confVal)
{
    if (0 == strncmp(confVal, "LOG_", 4)) {
        return confVal+4;
    } else {
        return confVal;
    }
}



/*
 * snoopy_configuration_strtoupper
 *
 * Description:
 *     Convert existing string to upper case
 *
 * Params:
 *     string   Pointer to string to change and to be operated on
 *
 * Return:
 *     void
 */
void snoopy_configuration_strtoupper (char *s)
{
    while (*s) {
        if ((*s >= 'a' ) && (*s <= 'z')) {
            *s -= ('a'-'A');
        }
        s++;
    }
}
