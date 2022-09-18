#pragma once

#include "redismodule.h"
#include "rmutil/sds.h"
#include "query_error.h"

enum RSTimeoutPolicy {
  TimeoutPolicy_Default = 0,  // Defer to global config
  TimeoutPolicy_Return,       // Return what we have on timeout
  TimeoutPolicy_Fail,         // Just fail without returning anything
  TimeoutPolicy_Invalid       // Not a real value
};

enum GCPolicy { GCPolicy_Fork = 0, GCPolicy_Sync };

const char *TimeoutPolicy_ToString(RSTimeoutPolicy);

/**
 * Returns TimeoutPolicy_Invalid if the string could not be parsed
 */
RSTimeoutPolicy TimeoutPolicy_Parse(const char *s, size_t n);

static inline const char *GCPolicy_ToString(GCPolicy policy) {
  switch (policy) {
    case GCPolicy_Sync:
      return "sync";
    case GCPolicy_Fork:
      return "fork";
    default:          // LCOV_EXCL_LINE cannot be reached
      return "huh?";  // LCOV_EXCL_LINE cannot be reached
  }
}

struct RSConfig;

struct RSConfigVar {
  const char *name;
  const char *helpText;
  // Whether this configuration option can be modified after initial loading
  int (*setValue)(RSConfig *, ArgsCursor *, QueryError *);
  sds (*getValue)(const RSConfig *);
  uint32_t flags;
};

#define RS_MAX_CONFIG_VARS 255
struct RSConfigOptions {
  RSConfigVar vars[RS_MAX_CONFIG_VARS];
  struct RSConfigOptions *next;
};

/* RSConfig is a global configuration struct for the module, it can be included from each file,
 * and is initialized with user config options during module statrtup */
struct RSConfig {
  // Version of Redis server
  int serverVersion;
  // Use concurrent serach (default: 1, disable with SAFEMODE)
  int concurrentMode;
  // If not null, this points at a .so file of an extension we try to load (default: NULL)
  const char *extLoad;
  // Path to friso.ini for chinese dictionary file
  const char *frisoIni;
  // If this is set, GC is enabled on all indexes (default: 1, disable with NOGC)
  int enableGC;

  // The minimal number of characters we allow expansion for in a prefix search. Default: 2
  long long minTermPrefix;

  // The maximal number of expansions we allow for a prefix. Default: 200
  long long maxPrefixExpansions;

  // The maximal amount of time a single query can take before timing out, in milliseconds.
  // 0 means unlimited
  long long queryTimeoutMS;

  // Number of rows to read from a cursor if not specified
  long long cursorReadSize;

  // Maximum idle time for a cursor. Users can use shorter lifespans, but never
  // longer ones
  long long cursorMaxIdle;

  long long timeoutPolicy;

  size_t maxDocTableSize;
  size_t maxSearchResults;
  size_t searchPoolSize;
  size_t indexPoolSize;
  int poolSizeNoAuto;  // Don't auto-detect pool size

  size_t gcScanSize;

  size_t minPhoneticTermLen;

  GCPolicy gcPolicy;
  size_t forkGcRunIntervalSec;
  size_t forkGcCleanThreshold;
  size_t forkGcRetryInterval;
  size_t forkGcSleepBeforeExit;

  // Chained configuration data
  void *chainedConfig;

  long long maxResultsToUnsortedMode;

  int noMemPool;

  // compress double to float
  int numericCompress;

  // FT.ADD with REPLACE deletes old field
  int replaceDeleteField;

  void DumpProto(const RSConfigOptions *options, const char *name,
                 RedisModuleCtx *ctx, int isHelp) const;

  int SetOption(RSConfigOptions *options, const char *name, RedisModuleString **argv,
                int argc, size_t *offset, QueryError *status);

  sds GetInfoString() const;
};

enum RSConfigVarFlags{
  RSCONFIGVAR_F_IMMUTABLE = 0x01,
  RSCONFIGVAR_F_MODIFIED = 0x02,
  RSCONFIGVAR_F_FLAG = 0x04,
  RSCONFIGVAR_F_SHORTHAND = 0x08
};

// global config extern references
extern RSConfig RSGlobalConfig;
extern RSConfigOptions RSGlobalConfigOptions;

// Read configuration from redis module arguments into the global config object.
// Return REDISMODULE_ERR and sets an error message if something is invalid.
int ReadConfig(RedisModuleString **argv, int argc, char **err);

#define DEFAULT_DOC_TABLE_SIZE 1000000
#define MAX_DOC_TABLE_SIZE 100000000
#define CONCURRENT_SEARCH_POOL_DEFAULT_SIZE 20
#define CONCURRENT_INDEX_POOL_DEFAULT_SIZE 8
#define CONCURRENT_INDEX_MAX_POOL_SIZE 200  // Maximum number of threads to create
#define GC_SCANSIZE 100
#define DEFAULT_MIN_PHONETIC_TERM_LEN 3
#define DEFAULT_FORK_GC_RUN_INTERVAL 10
#define DEFAULT_MAX_RESULTS_TO_UNSORTED_MODE 1000
#define SEARCH_REQUEST_RESULTS_MAX 1000000

// default configuration
#define RS_DEFAULT_CONFIG  \
  {  \
    concurrentMode: 0,  \
    extLoad: NULL,  \
    enableGC: 1,  \
    minTermPrefix: 2,  \
    maxPrefixExpansions: 200,  \
    queryTimeoutMS: 500,  \
    cursorReadSize: 1000,  \
    cursorMaxIdle: 300000,  \
    timeoutPolicy: TimeoutPolicy_Return,  \
    maxDocTableSize: DEFAULT_DOC_TABLE_SIZE,  \
    maxSearchResults: SEARCH_REQUEST_RESULTS_MAX, \
    searchPoolSize: CONCURRENT_SEARCH_POOL_DEFAULT_SIZE,  \
    indexPoolSize: CONCURRENT_INDEX_POOL_DEFAULT_SIZE,  \
    poolSizeNoAuto: 0,  \
    gcScanSize: GC_SCANSIZE,  \
    minPhoneticTermLen: DEFAULT_MIN_PHONETIC_TERM_LEN,  \
    gcPolicy: GCPolicy_Fork,  \
    forkGcRunIntervalSec: DEFAULT_FORK_GC_RUN_INTERVAL,  \
    forkGcCleanThreshold: 0,  \
    forkGcRetryInterval: 5,  \
    forkGcSleepBeforeExit: 0,  \
    maxResultsToUnsortedMode: DEFAULT_MAX_RESULTS_TO_UNSORTED_MODE,  \
    noMemPool: 0,  \
    numericCompress: 1, \
    replaceDeleteField: 0, \
  }

#define REDIS_ARRAY_LIMIT 7
#define NO_REPLY_DEPTH_LIMIT 0x00060020

static inline int isFeatureSupported(int feature) {
  return feature <= RSGlobalConfig.serverVersion;
}

