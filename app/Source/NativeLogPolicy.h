#pragma once
#include <string.h>
// Informational per-frame traces only. Warnings/errors are retained or sampled separately.
static inline int pg_verbose_log_class(const char *s){
 if(!strncmp(s,"[cutscene] Updating cutscene time.",strlen("[cutscene] Updating cutscene time.")))return 1;
 if(!strncmp(s,"[scrCommandHash debug]  Insert to bucket",strlen("[scrCommandHash debug]  Insert to bucket")))return 2;
 if(!strncmp(s,"Warning: [cutscene]",strlen("Warning: [cutscene]"))&&strstr(s,"Audio time isn't progressing"))return 3;
 return 0;
}
