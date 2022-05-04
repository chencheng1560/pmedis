#include "pmedis.h"
/*
  if the old value can not convert to a long long number,
      new_val_len to 0, new_val indicates error message.
*/
void modifyll(const char *old_val, size_t old_val_len, char **new_val,
              size_t *new_val_len, void *n_pointer) {
  assert(n_pointer);
  assert(old_val_len == sizeof(long long));

  /* Check whether it is a number */
  long long l_old_val, l_new_val;
  if (0 == string2ll(old_val, old_val_len, &l_old_val)) {
    //*new_val_len=0;
    //*new_val = (char*)malloc(11);
    // char msg[11]="wrong type";
    // memset(*new_val, 0, 11);
    // memcpy(*new_val, msg, 11);
    return;
  }

  long long incr = *((long long *)n_pointer);
  if ((incr < 0 && l_old_val < 0 && incr < (LLONG_MIN - l_old_val)) ||
      (incr > 0 && l_old_val > 0 && incr > (LLONG_MAX - l_old_val))) {
    /* updated value will overflow */
    //*new_val_len=0;
    //*new_val = (char*)malloc(38);
    // char msg[38]="increment or decrement would overflow";
    // memset(*new_val, 0, 38);
    // memcpy(*new_val, msg, 38);
    return;
  }
  l_new_val = l_old_val + incr;

  // set value
  *new_val = (char *)malloc(sizeof(long long));
  *new_val_len = sizeof(long long);
  memcpy(*new_val, &l_new_val, sizeof(long long));
}

/*
int IncN(const char *old_val, size_t old_val_len, char **new_val,
         size_t *new_val_len, void *args_pointer) {
  // assert(args_pointer);
  IncNArgs *args = (IncNArgs *)args_pointer;
  *new_val = (char *)malloc(sizeof(int64_t));
  if (*new_val == NULL) {
    return KVDK_MODIFY_ABORT;
  }

  *new_val_len = sizeof(int64_t);
  int64_t old_num;
  if (old_val == NULL) {
    old_num = 0;
  } else {
    if (old_val_len != sizeof(int64_t)) {
      return KVDK_MODIFY_ABORT;
    }
    // assert(old_val_len == sizeof(int64_t));
    memcpy(&old_num, old_val, sizeof(int64_t));
  }
  args->result = old_num + args->incr_by;
  memcpy(*new_val, &args->result, sizeof(int64_t));
  return KVDK_MODIFY_WRITE;
}
*/

int IncN(const char *old_val, size_t old_val_len, char **new_val,
         size_t *new_val_len, void *args_pointer) {
  // assert(args_pointer);
  IncNArgs *args = (IncNArgs *)args_pointer;
  long long incr = args->incr_by;
  long long l_old_val, l_new_val;
  if (old_val == NULL) {
    l_old_val = 0;
  } else {
    if (0 == string2ll(old_val, old_val_len, &l_old_val)) {
      args->err_no = INVALID_NUMBER;
      return KVDK_MODIFY_ABORT;
    }
  }
  if ((incr < 0 && l_old_val < 0 && incr < (LLONG_MIN - l_old_val)) ||
      (incr > 0 && l_old_val > 0 && incr > (LLONG_MAX - l_old_val))) {
    args->err_no = NUMBER_OVERFLOW;
    return KVDK_MODIFY_ABORT;
  }
  l_new_val = l_old_val + incr;

  *new_val = (char *)malloc(32);
  if (*new_val == NULL) {
    args->err_no = MALLOC_ERR;
    return KVDK_MODIFY_ABORT;
  }
  *new_val_len = ll2string(*new_val, LLSTR_SIZE, l_new_val);
  if (0 == *new_val_len) {
    args->err_no = MALLOC_ERR;
    return KVDK_MODIFY_ABORT;
  }
  args->result = l_new_val;
  return KVDK_MODIFY_WRITE;
}

KVDKStatus incrDecr(RedisModuleCtx *ctx, const char *key_str, size_t key_len,
                    IncNArgs *args) {
  KVDKWriteOptions *write_option = KVDKCreateWriteOptions();
  KVDKStatus s =
      KVDKModify(engine, key_str, key_len, IncN, args, free, write_option);
  KVDKDestroyWriteOptions(write_option);
  return s;
}

int pmIncrCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  size_t key_len;
  if (argc != 2) return RedisModule_WrongArity(ctx);
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  IncNArgs args;
  args.incr_by = 1;
  KVDKStatus s = incrDecr(ctx, key_str, key_len, &args);
  if (s == Abort)
    return RedisModule_ReplyWithError(
        ctx, "value is not an integer or out of range");
  else
    return RedisModule_ReplyWithLongLong(ctx, args.result);
}
int pmDecrCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  size_t key_len;
  if (argc != 2) return RedisModule_WrongArity(ctx);
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  IncNArgs args;
  args.incr_by = -1;
  KVDKStatus s = incrDecr(ctx, key_str, key_len, &args);
  if (s == Abort)
    return RedisModule_ReplyWithError(
        ctx, "value is not an integer or out of range");
  else
    return RedisModule_ReplyWithLongLong(ctx, args.result);
}

int pmIncrbyCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 3) return RedisModule_WrongArity(ctx);
  size_t key_len;
  long long incr;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  if (REDISMODULE_ERR == RedisModule_StringToLongLong(argv[2], &incr)) {
    return RedisModule_ReplyWithError(
        ctx, "ERR value is not an integer or out of range");
  }
  IncNArgs args;
  args.incr_by = incr;
  KVDKStatus s = incrDecr(ctx, key_str, key_len, &args);
  if (s == Abort)
    return RedisModule_ReplyWithError(
        ctx, "value is not an integer or out of range");
  else
    return RedisModule_ReplyWithLongLong(ctx, args.result);
}

int pmDecrbyCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 3) return RedisModule_WrongArity(ctx);
  size_t key_len;
  long long decr;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  if (REDISMODULE_ERR == RedisModule_StringToLongLong(argv[2], &decr)) {
    return RedisModule_ReplyWithError(
        ctx, "ERR value is not an integer or out of range");
  }
  IncNArgs args;
  args.incr_by = -decr;
  KVDKStatus s = incrDecr(ctx, key_str, key_len, &args);
  if (s == Abort)
    return RedisModule_ReplyWithError(
        ctx, "value is not an integer or out of range");
  else
    return RedisModule_ReplyWithLongLong(ctx, args.result);
}

// Wait: KVDK Read-modify-write
int pmIncrbyfloatCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc) {
  size_t key_len;
  long double incr, ori_value;
  if (argc != 3) return RedisModule_WrongArity(ctx);
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);

  if (REDISMODULE_ERR == RedisModule_StringToLongDouble(argv[2], &incr)) {
    return RedisModule_ReplyWithError(
        ctx, "ERR value is not an float or out of range");
  }

  /*
  size_t incr_str_len;
  const char *incr_str = RedisModule_StringPtrLen(argv[2], &incr_str_len);
  if (!string2ld(incr_str, incr_str_len, &incr))
    return RedisModule_ReplyWithError(
        ctx, "ERR value is not an float or out of range");
  */
  /* Get String from KVDK */
  /* Check the original value is float or not */
  /*
  if (!string2ld(ori_value_str, ori_value_str_len, &ori_value))
    return RedisModule_ReplyWithError(
        ctx, "ERR value is not an float or out of range");
  */
  (void)key_str;
  (void)ori_value;
  return RedisModule_ReplyWithError(ctx, MSG_WAIT_KVDK_FUNC_SUPPORT);
}

int pmAppendCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  size_t key_len, append_val_len, ori_val_len;
  char *ori_val_str;
  size_t target_len;
  if (argc != 3) return RedisModule_WrongArity(ctx);
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  const char *append_val_str =
      RedisModule_StringPtrLen(argv[2], &append_val_len);
  char *target_str = NULL;

  KVDKStatus s = KVDKGet(engine, key_str, key_len, &ori_val_len, &ori_val_str);

  if (s != Ok && s != NotFound) {
    /* Something Err in KVDK */
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  } else if (s == NotFound) {
    /* Create the key */
    target_len = append_val_len;
    target_str = (char *)append_val_str;
  } else {
    /* Key exists, check type */
    // TODO: require KVDK support input key, output: key-type
    // TODO: if(type != OBJ_STRING) return RedisModule_ReplyWithError(ctx,"ERR
    // Wrong Type");
    /* Append the value */
    target_len = append_val_len + ori_val_len;
    target_str =
        safeStrcat(ori_val_str, ori_val_len, append_val_str, append_val_len);
  }
  KVDKWriteOptions *write_option = KVDKCreateWriteOptions();
  s = KVDKSet(engine, key_str, key_len, target_str, target_len, write_option);
  free(ori_val_str);  // free memory allocated by KVDKGet
  RedisModule_Free(target_str);
  // free(target_str);

  if (s != Ok) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  }
  return RedisModule_ReplyWithLongLong(ctx, target_len);
}

int pmStrlenCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 2) return RedisModule_WrongArity(ctx);
  size_t key_len, val_len, str_len;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  char *val_str;
  KVDKStatus s = KVDKGet(engine, key_str, key_len, &val_len, &val_str);
  if (s != Ok && s != NotFound) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  } else if (s == NotFound) {
    /* Key not exist*/
    str_len = 0;
  } else {
    /* Key exists, check type */
    // TODO: require KVDK support input key, output: key-type
    // TODO: if(type != OBJ_STRING) return RedisModule_ReplyWithError(ctx,"ERR
    // Wrong Type");
    /* Append the value */
    str_len = val_len;
  }
  return RedisModule_ReplyWithLongLong(ctx, str_len);
}

// Wait KVDK Transaction Support
int pmMsetGenericCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                         int argc, int nx) {
  if (0 == (argc % 2))
    return RedisModule_ReplyWithError(ctx,
                                      "wrong number of arguments for MSET");
  int j;
  size_t key_len, val_len;
  KVDKStatus s;
  char *val_str;
  /* Handle the NX flag. The MSETNX semantic is to return zero and don't
   * set anything if at least one key already exists. */
  if (nx) {
    for (j = 1; j < argc; j += 2) {
      const char *key_str = RedisModule_StringPtrLen(argv[j], &key_len);
      s = KVDKGet(engine, key_str, key_len, &val_len, &val_str);
      if (s == Ok) {
        return RedisModule_ReplyWithLongLong(ctx, 0);
      }
    }
  }
  /*
  KVDKWriteOptions *write_option = KVDKCreateWriteOptions();
  for (j = 1; j < argc; j += 2) {
    const char *key_str = RedisModule_StringPtrLen(argv[j], &key_len);
    const char *val_str = RedisModule_StringPtrLen(argv[j + 1], &val_len);
    s = KVDKSet(engine, key_str, key_len, val_str, val_len, write_option);
    if (s != Ok) {
      return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
    }
  }
  */
  KVDKWriteBatch *kvdk_wb = KVDKWriteBatchCreate();
  for (j = 1; j < argc; j += 2) {
    const char *key_str = RedisModule_StringPtrLen(argv[j], &key_len);
    const char *val_str = RedisModule_StringPtrLen(argv[j + 1], &val_len);
    KVDKWriteBatchPut(kvdk_wb, key_str, strlen(key_str), val_str,
                      strlen(val_str));
  }
  s = KVDKWrite(engine, kvdk_wb);
  KVDKWriteBatchDestory(kvdk_wb);
  if (s != Ok) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  }
  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

int pmMsetCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  return pmMsetGenericCommand(ctx, argv, argc, 0);
}

int pmMsetnxCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  return pmMsetGenericCommand(ctx, argv, argc, 1);
}

int pmMgetCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc < 2) return RedisModule_WrongArity(ctx);
  size_t key_len, val_len;
  int i;
  RedisModule_ReplyWithArray(ctx, argc - 1);

  for (i = 1; i < argc; ++i) {
    const char *key_str = RedisModule_StringPtrLen(argv[i], &key_len);
    char *val_str;
    KVDKStatus s = KVDKGet(engine, key_str, key_len, &val_len, &val_str);
    if (s != Ok && s != NotFound) {
      return RedisModule_ReplyWithError(ctx, "MGET KVDKGet Return Err");
    } else if (s == NotFound) {
      RedisModule_ReplyWithStringBuffer(ctx, "(nil)", 5);
    } else {
      RedisModule_ReplyWithStringBuffer(ctx, val_str, val_len);
      free(val_str);
    }
  }
  return REDISMODULE_OK;
}

int pmGetGenericCommand(RedisModuleCtx *ctx, const char *key_str,
                        size_t key_len, KVDKStatus *s) {
  size_t val_len;
  char *val_str;
  *s = KVDKGet(engine, key_str, key_len, &val_len, &val_str);
  if (*s != Ok && *s != NotFound) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[*s]);
  } else if (NotFound == *s) {
    return RedisModule_ReplyWithNull(ctx);
  }
  RedisModule_ReplyWithStringBuffer(ctx, val_str, val_len);
  free(val_str);
  return REDISMODULE_OK;
}

int pmGetdelCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 2) return RedisModule_WrongArity(ctx);
  size_t key_len;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  KVDKStatus sGet, sDel;
  pmGetGenericCommand(ctx, key_str, key_len, &sGet);
  if (Ok == sGet) {
    sDel = KVDKDelete(engine, key_str, key_len);
    if (sDel != Ok) {
      return RedisModule_ReplyWithError(ctx, enum_to_str[sDel]);
    }
  }
  return REDISMODULE_OK;
}

/*
 * PMGETEX <key> [PERSIST][EX seconds][PX milliseconds][EXAT
 * seconds-timestamp][PXAT milliseconds-timestamp]
 *
 * The getexCommand() function implements extended options and variants of the
 * GET command. Unlike GET command this command is not read-only.
 *
 * The default behavior when no options are specified is same as GET and does
 * not alter any TTL.
 *
 * Only one of the below options can be used at a given time.
 *
 * 1. PERSIST removes any TTL associated with the key.
 * 2. EX Set expiry TTL in seconds.
 * 3. PX Set expiry TTL in milliseconds.
 * 4. EXAT Same like EX instead of specifying the number of seconds representing
 * the TTL (time to live), it takes an absolute Unix timestamp
 * 5. PXAT Same like PX instead of specifying the number of milliseconds
 * representing the TTL (time to live), it takes an absolute Unix timestamp
 *
 * Command would either return the bulk string, error or nil.
 */
// Wait for KVDK TTL
int pmGetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  return RedisModule_ReplyWithError(ctx, MSG_WAIT_KVDK_FUNC_SUPPORT);
}
// Wait for KVDK TTL
int pmGetCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 2) return RedisModule_WrongArity(ctx);
  size_t key_len;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  KVDKStatus s;
  return pmGetGenericCommand(ctx, key_str, key_len, &s);
}

// Wait for KVDK TTL
int pmSetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 4) return RedisModule_WrongArity(ctx);
  return RedisModule_ReplyWithError(ctx, MSG_WAIT_KVDK_FUNC_SUPPORT);
}

// Wait for KVDK TTL
int pmPsetexCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 4) return RedisModule_WrongArity(ctx);
  return RedisModule_ReplyWithError(ctx, MSG_WAIT_KVDK_FUNC_SUPPORT);
}

int pmSetCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 3) return RedisModule_WrongArity(ctx);
  size_t key_len;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  size_t val_len;
  const char *val_str = RedisModule_StringPtrLen(argv[2], &val_len);
  KVDKWriteOptions *write_option = KVDKCreateWriteOptions();
  KVDKStatus s =
      KVDKSet(engine, key_str, key_len, val_str, val_len, write_option);
  KVDKDestroyWriteOptions(write_option);
  if (s != Ok) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  }
  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

int pmSetnxCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 3) return RedisModule_WrongArity(ctx);
  size_t key_len, val_len, ori_val_len;
  const char *key_str = RedisModule_StringPtrLen(argv[1], &key_len);
  char *ori_val_str;
  KVDKStatus s = KVDKGet(engine, key_str, key_len, &ori_val_len, &ori_val_str);
  if (s == Ok) {
    return RedisModule_ReplyWithLongLong(ctx, 0);
  }
  const char *val_str = RedisModule_StringPtrLen(argv[2], &val_len);
  KVDKWriteOptions *write_option = KVDKCreateWriteOptions();
  s = KVDKSet(engine, key_str, key_len, val_str, val_len, write_option);
  if (s != Ok) {
    return RedisModule_ReplyWithError(ctx, enum_to_str[s]);
  }
  return RedisModule_ReplyWithLongLong(ctx, 1);
}