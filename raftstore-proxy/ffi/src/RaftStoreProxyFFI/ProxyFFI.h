#pragma once

#include "ColumnFamily.h"

namespace DB {

struct EngineStoreServerWrap;

enum class SpecialCppPtrType : uint32_t {
  None = 0,
  TupleOfRawCppPtr = 1,
  ArrayOfRawCppPtr = 2,
};

enum class EngineStoreApplyRes : uint32_t {
  None = 0,
  Persist,
  NotFound,
};

enum class WriteCmdType : uint8_t {
  Put = 0,
  Del,
};

struct BaseBuffView {
  const char *data;
  const uint64_t len;
};

struct RaftCmdHeader {
  uint64_t region_id;
  uint64_t index;
  uint64_t term;
};

struct WriteCmdsView {
  const BaseBuffView *keys;
  const BaseBuffView *vals;
  const WriteCmdType *cmd_types;
  const ColumnFamilyType *cmd_cf;
  const uint64_t len;
};

struct FsStats {
  uint64_t used_size;
  uint64_t avail_size;
  uint64_t capacity_size;

  uint8_t ok;
};

struct StoreStats {
  FsStats fs_stats;

  uint64_t engine_bytes_written;
  uint64_t engine_keys_written;
  uint64_t engine_bytes_read;
  uint64_t engine_keys_read;
};

enum class SSTFormatKind : uint16_t {
  KIND_SST = 0,
  KIND_TABLET,
};

enum class EngineIteratorSeekType : uint16_t {
  Key = 0,
  First,
  Last,
};

enum class RaftProxyStatus : uint8_t {
  Idle = 0,
  Running,
  Stopped,
};

enum class EngineStoreServerStatus : uint8_t {
  Idle = 0,
  Running,
  Stopping,  // let raft-proxy stop all services, but encryption module is still
             // working.
  Terminated,  // after all services stopped in engine-store, let raft-proxy
               // shutdown.
};

enum class RaftstoreVer : uint8_t {
  Uncertain = 0,
  V1,
  V2,
};

using RawCppPtrType = uint32_t;
using RawRustPtrType = uint32_t;

struct RawRustPtr {
  RawVoidPtr ptr;
  RawRustPtrType type;
};

struct RawCppPtr {
  RawVoidPtr ptr;
  RawCppPtrType type;
};

struct CppStrWithView {
  RawCppPtr inner;
  BaseBuffView view;
};

struct PageAndCppStrWithView {
  RawCppPtr page;
  RawCppPtr key;
  BaseBuffView page_view;
  BaseBuffView key_view;
};

struct RawCppPtrCarr {
  RawVoidPtr inner;
  const uint64_t len;
  RawCppPtrType type;
};

// An tuple of pointers, like `void **`,
// Can be used to represent structures created from C side.
struct RawCppPtrTuple {
  RawCppPtr *inner;
  const uint64_t len;
};

// An array of pointers(same type), like `T **`,
// Can be used to represent arrays created from C side.
struct RawCppPtrArr {
  RawVoidPtr *inner;
  const uint64_t len;
  RawCppPtrType type;
};

enum class HttpRequestStatus : uint8_t {
  Ok = 0,
  ErrorParam,
};

struct HttpRequestRes {
  HttpRequestStatus status;
  CppStrWithView res;
};

struct CppStrVecView {
  const BaseBuffView *view;
  uint64_t len;
};

struct FileEncryptionInfoRaw;
enum class EncryptionMethod : uint8_t;

struct SSTView {
  ColumnFamilyType type;
  BaseBuffView path;
};

struct SSTViewVec {
  const SSTView *views;
  const uint64_t len;
};

struct RaftStoreProxyPtr {
  ConstRawVoidPtr inner;
};

struct SSTReaderPtr {
  RawVoidPtr inner;
  SSTFormatKind kind;
};

struct RustStrWithView {
  // It is a view of something inside `inner`.
  BaseBuffView buff;
  // Should never be accessed on C++ side.
  // Call fn_gc_rust_ptr to this after `buffs` is no longer used.
  RawRustPtr inner;
};

struct RustStrWithViewVec {
  // It is a view of something inside `inner`.
  const BaseBuffView *buffs;
  // It is the length of view.
  uint64_t len;
  // Should never be accessed on C++ side.
  // Call fn_gc_rust_ptr to this after `buffs` is no longer used.
  RawRustPtr inner;
};

struct SSTReaderInterfaces {
  SSTReaderPtr (*fn_get_sst_reader)(SSTView, RaftStoreProxyPtr);
  uint8_t (*fn_remained)(SSTReaderPtr, ColumnFamilyType);
  BaseBuffView (*fn_key)(SSTReaderPtr, ColumnFamilyType);
  BaseBuffView (*fn_value)(SSTReaderPtr, ColumnFamilyType);
  void (*fn_next)(SSTReaderPtr, ColumnFamilyType);
  void (*fn_gc)(SSTReaderPtr, ColumnFamilyType);
  SSTFormatKind (*fn_kind)(SSTReaderPtr, ColumnFamilyType);
  void (*fn_seek)(SSTReaderPtr, ColumnFamilyType, EngineIteratorSeekType,
                  BaseBuffView);
  uint64_t (*fn_approx_size)(SSTReaderPtr, ColumnFamilyType);
  RustStrWithViewVec (*fn_get_split_keys)(SSTReaderPtr, uint64_t splits_count);
};

struct CloudStorageEngineInterfaces {
  bool (*fn_get_keyspace_encryption)(RaftStoreProxyPtr, uint32_t);
  RawCppStringPtr (*fn_get_master_key)(RaftStoreProxyPtr);
};

enum class MsgPBType : uint32_t {
  ReadIndexResponse = 0,
  ServerInfoResponse,
  RegionLocalState,
};

enum class KVGetStatus : uint32_t {
  Ok = 0,
  Error,
  NotFound,
};

enum class FastAddPeerStatus : uint32_t {
  Ok = 0,
  WaitForData,
  OtherError,
  NoSuitable,
  BadData,
  FailedInject,
  Canceled,  // Cancel from outside the FAP workers
};

enum class PrehandledSnapshotType : uint8_t {
  None = 0,
  Legacy,
  FAPCheckpoint,
};

struct FastAddPeerRes {
  FastAddPeerStatus status;
  CppStrWithView apply_state;
  CppStrWithView region;
};

enum class FapSnapshotState : uint32_t {
  NotFound,
  Persisted,
  Other,
};

enum class ConfigJsonType : uint64_t { ProxyConfigAddressed = 1 };

struct RaftStoreProxyFFIHelper {
  RaftStoreProxyPtr proxy_ptr;
  RaftProxyStatus (*fn_handle_get_proxy_status)(RaftStoreProxyPtr);
  uint8_t (*fn_is_encryption_enabled)(RaftStoreProxyPtr);
  EncryptionMethod (*fn_encryption_method)(RaftStoreProxyPtr);
  FileEncryptionInfoRaw (*fn_handle_get_file)(RaftStoreProxyPtr, BaseBuffView);
  FileEncryptionInfoRaw (*fn_handle_new_file)(RaftStoreProxyPtr, BaseBuffView);
  FileEncryptionInfoRaw (*fn_handle_delete_file)(RaftStoreProxyPtr,
                                                 BaseBuffView);
  FileEncryptionInfoRaw (*fn_handle_link_file)(RaftStoreProxyPtr, BaseBuffView,
                                               BaseBuffView);
  void (*fn_handle_batch_read_index)(
      RaftStoreProxyPtr, CppStrVecView, RawVoidPtr, uint64_t,
      void (*fn_insert_batch_read_index_resp)(RawVoidPtr, BaseBuffView,
                                              uint64_t));  // To remove
  SSTReaderInterfaces sst_reader_interfaces;
  CloudStorageEngineInterfaces cloud_storage_engine_interfaces;

  uint32_t (*fn_server_info)(RaftStoreProxyPtr, BaseBuffView, RawVoidPtr);
  RawRustPtr (*fn_make_read_index_task)(RaftStoreProxyPtr, BaseBuffView);
  RawRustPtr (*fn_make_async_waker)(void (*wake_fn)(RawVoidPtr),
                                    RawCppPtr data);
  uint8_t (*fn_poll_read_index_task)(RaftStoreProxyPtr, RawVoidPtr task,
                                     RawVoidPtr resp, RawVoidPtr waker);
  void (*fn_gc_rust_ptr)(RawVoidPtr, RawRustPtrType);
  RawRustPtr (*fn_make_timer_task)(uint64_t millis);
  uint8_t (*fn_poll_timer_task)(RawVoidPtr task, RawVoidPtr waker);
  KVGetStatus (*fn_get_region_local_state)(RaftStoreProxyPtr,
                                           uint64_t region_id, RawVoidPtr data,
                                           RawCppStringPtr *error_msg);
  RaftstoreVer (*fn_get_cluster_raftstore_version)(RaftStoreProxyPtr,
                                                   uint8_t refresh_strategy,
                                                   int64_t timeout_ms);
  void (*fn_notify_compact_log)(RaftStoreProxyPtr, uint64_t region_id,
                                uint64_t compact_index, uint64_t compact_term,
                                uint64_t applied_index);
  RustStrWithView (*fn_get_config_json)(RaftStoreProxyPtr, ConfigJsonType kind);
};

struct PageStorageInterfaces {
  RawCppPtr (*fn_create_write_batch)(const EngineStoreServerWrap *);
  void (*fn_wb_put_page)(RawVoidPtr, BaseBuffView, BaseBuffView);
  void (*fn_wb_del_page)(RawVoidPtr, BaseBuffView);
  uint64_t (*fn_get_wb_size)(RawVoidPtr);
  uint8_t (*fn_is_wb_empty)(RawVoidPtr);
  void (*fn_handle_merge_wb)(RawVoidPtr, RawVoidPtr);
  void (*fn_handle_clear_wb)(RawVoidPtr);
  void (*fn_handle_consume_wb)(const EngineStoreServerWrap *, RawVoidPtr);
  CppStrWithView (*fn_handle_read_page)(const EngineStoreServerWrap *,
                                        BaseBuffView);
  RawCppPtrCarr (*fn_handle_scan_page)(const EngineStoreServerWrap *,
                                       BaseBuffView, BaseBuffView);
  CppStrWithView (*fn_handle_get_lower_bound)(const EngineStoreServerWrap *,
                                              BaseBuffView);
  uint8_t (*fn_is_ps_empty)(const EngineStoreServerWrap *);
  void (*fn_handle_purge_ps)(const EngineStoreServerWrap *);
};

enum class ReportThreadAllocateInfoType : uint64_t {
  Reset = 0,
  Remove,
  AllocPtr,
  DeallocPtr,
};

struct ReportThreadAllocateInfoBatch {
  uint64_t alloc;
  uint64_t dealloc;
};

struct EngineStoreServerHelper {
  uint32_t magic_number;  // use a very special number to check whether this
                          // struct is legal
  uint64_t version;       // version of function interface
  //

  EngineStoreServerWrap *inner;
  PageStorageInterfaces ps;
  RawCppPtr (*fn_gen_cpp_string)(BaseBuffView);
  EngineStoreApplyRes (*fn_handle_write_raft_cmd)(const EngineStoreServerWrap *,
                                                  WriteCmdsView, RaftCmdHeader);
  EngineStoreApplyRes (*fn_handle_admin_raft_cmd)(const EngineStoreServerWrap *,
                                                  BaseBuffView, BaseBuffView,
                                                  RaftCmdHeader);
  uint8_t (*fn_need_flush_data)(EngineStoreServerWrap *, uint64_t);
  uint8_t (*fn_try_flush_data)(EngineStoreServerWrap *, uint64_t region_id,
                               uint8_t flush_pattern, uint64_t index,
                               uint64_t term, uint64_t truncated_index,
                               uint64_t truncated_term);
  void (*fn_atomic_update_proxy)(EngineStoreServerWrap *,
                                 RaftStoreProxyFFIHelper *);
  void (*fn_handle_destroy)(EngineStoreServerWrap *, uint64_t);
  EngineStoreApplyRes (*fn_handle_ingest_sst)(EngineStoreServerWrap *,
                                              SSTViewVec, RaftCmdHeader);
  StoreStats (*fn_handle_compute_store_stats)(EngineStoreServerWrap *);
  EngineStoreServerStatus (*fn_handle_get_engine_store_server_status)(
      EngineStoreServerWrap *);
  RawCppPtr (*fn_pre_handle_snapshot)(EngineStoreServerWrap *, BaseBuffView,
                                      uint64_t, SSTViewVec, uint64_t, uint64_t);
  void (*fn_apply_pre_handled_snapshot)(EngineStoreServerWrap *, RawVoidPtr,
                                        RawCppPtrType);
  void (*fn_abort_pre_handle_snapshot)(EngineStoreServerWrap *, uint64_t,
                                       uint64_t);
  void (*fn_release_pre_handled_snapshot)(EngineStoreServerWrap *, RawVoidPtr,
                                          RawCppPtrType);
  uint8_t (*fn_apply_fap_snapshot)(EngineStoreServerWrap *, uint64_t, uint64_t,
                                   uint8_t, uint64_t, uint64_t);
  HttpRequestRes (*fn_handle_http_request)(EngineStoreServerWrap *,
                                           BaseBuffView path,
                                           BaseBuffView query,
                                           BaseBuffView body);
  uint8_t (*fn_check_http_uri_available)(BaseBuffView);
  void (*fn_gc_raw_cpp_ptr)(RawVoidPtr, RawCppPtrType);
  void (*fn_gc_raw_cpp_ptr_carr)(RawVoidPtr, RawCppPtrType, uint64_t);
  void (*fn_gc_special_raw_cpp_ptr)(RawVoidPtr, uint64_t, SpecialCppPtrType);
  CppStrWithView (*fn_get_config)(EngineStoreServerWrap *, uint8_t full);
  void (*fn_set_store)(EngineStoreServerWrap *, BaseBuffView);
  void (*fn_set_pb_msg_by_bytes)(MsgPBType type, RawVoidPtr ptr,
                                 BaseBuffView buff);
  void (*fn_handle_safe_ts_update)(EngineStoreServerWrap *, uint64_t region_id,
                                   uint64_t self_safe_ts,
                                   uint64_t leader_safe_ts);
  FastAddPeerRes (*fn_fast_add_peer)(EngineStoreServerWrap *,
                                     uint64_t region_id, uint64_t new_peer_id);
  BaseBuffView (*fn_get_lock_by_key)(const EngineStoreServerWrap *, uint64_t,
                                     BaseBuffView);
  FapSnapshotState (*fn_query_fap_snapshot_state)(EngineStoreServerWrap *,
                                                  uint64_t region_id,
                                                  uint64_t new_peer_id,
                                                  uint64_t, uint64_t);
  void (*fn_clear_fap_snapshot)(EngineStoreServerWrap *, uint64_t region_id);
  bool (*fn_kvstore_region_exists)(EngineStoreServerWrap *, uint64_t region_id);
  void (*fn_report_thread_allocate_info)(EngineStoreServerWrap *,
                                         uint64_t thread_id, BaseBuffView name,
                                         ReportThreadAllocateInfoType type,
                                         uint64_t value);
  void (*fn_report_thread_allocate_batch)(EngineStoreServerWrap *,
                                          uint64_t thread_id, BaseBuffView name,
                                          ReportThreadAllocateInfoBatch data);
};

#ifdef __cplusplus
extern "C" {
#endif
// Basically same as ffi_server_info, but no need to setup ProxyHelper, only
// need to setup ServerHelper. Used when proxy not start.
uint32_t ffi_get_server_info_from_proxy(intptr_t, BaseBuffView, RawVoidPtr);
#ifdef __cplusplus
}
#endif

}  // namespace DB
