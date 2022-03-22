#define REDISMODULE_EXPERIMENTAL_API
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kvdk/engine.h"
#include "redismodule.h"

#define PMEM_FILE_SIZE 1 << 30
#define HASH_BUCKET_NUM 1 << 27
#define PMEM_SEG_BLOCKS 2 << 20
#define PMEM_BLOCK_SIZE 64
#define HASH_BUCKET_SIZE 128
#define NUM_BUCKETS_PER_SLOT 1
#define POPULATE_PMEM_SPACE 1U

extern const char *pmem_path;
extern KVDKEngine *engine;
extern KVDKConfigs *config;
extern const char *enum_to_str[];

typedef struct ThdPoolTask ThdPoolTask_t;
typedef struct ThdPool ThdPool_t;
extern ThdPool_t *thp;

extern int InitKVDK(RedisModuleCtx *ctx, RedisModuleString **argv, int argc);

extern int PmHello_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                int argc);
extern int HelloBlock_Reply(RedisModuleCtx *ctx, RedisModuleString **argv,
                            int argc);
extern int HelloBlock_Timeout(RedisModuleCtx *ctx, RedisModuleString **argv,
                              int argc);
extern void HelloBlock_FreeData(RedisModuleCtx *ctx, void *privdata);
extern void *HelloBlock_ThreadMain(void *arg);
extern void HelloBlock_Disconnected(RedisModuleCtx *ctx,
                                    RedisModuleBlockedClient *bc);
extern int HelloBlock_RedisCommand(RedisModuleCtx *ctx,
                                   RedisModuleString **argv, int argc);
extern void *HelloKeys_ThreadMain(void *arg);
extern int HelloKeys_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                  int argc);
extern int PmSleep_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                int argc);
// String commands
extern int pmAppendCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmIncrCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmDecrCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmIncrbyCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmIncrbyfloatCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                                int argc);
extern int pmDecrbyCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmStrlenCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmSetCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                        int argc);
extern int pmSetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmSetnxCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmPsetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmGetCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                        int argc);
extern int pmGetdelCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmGetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmMgetCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmMsetCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmMsetnxCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);

// List Commands
extern int pmRpushCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmLpushCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmRpushxCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmLpushxCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmLinsertCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                            int argc);
extern int pmRpopCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmLpopCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmBrpopCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmBrpoplpushCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                               int argc);
extern int pmBlmoveCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmBlpopCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmLlenCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmLindexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmLsetCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);

extern int pmLrangeCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                           int argc);
extern int pmLtrimCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
extern int pmLposCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmLremCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc);
extern int pmRpoplpushCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                              int argc);
extern int pmLmoveCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                          int argc);
// Thread pool
extern int thp_destroy(ThdPool_t *);
extern ThdPool_t *thp_create(int, int, int);
extern int thp_add(ThdPool_t *, void *func, RedisModuleBlockedClient *,
                   RedisModuleString **, int);