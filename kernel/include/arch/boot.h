#pragma once

/* Initial higher-half kernel mapping. */
#define EARLY_MAPPING_START 0xFFFFFFFF80000000ULL
#define EARLY_MAPPING_END (EARLY_MAPPING_START + 0x40000000ULL)
