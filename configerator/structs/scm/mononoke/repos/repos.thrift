// @generated SignedSource<<4601470c0a05b1f225f9db60c31fdbb0>>
// DO NOT EDIT THIS FILE MANUALLY!
// This file is a mechanical copy of the version in the configerator repo. To
// modify it, edit the copy in the configerator repo instead and copy it over by
// running the following in your fbcode directory:
//
// configerator-thrift-updater scm/mononoke/repos/repos.thrift

/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This software may be used and distributed according to the terms of the
 * GNU General Public License version 2.
 */

namespace py configerator.mononoke.repos

 // NOTICE:
 // Don't use 'defaults' for any of these values (e.g. 'bool enabled = true')
 // because these structs will be deserialized by serde in rust. The following
 // rules apply upon deserialization:
 //   1) specified default values are ignored, default values will always be
 //      the 'Default::default()' value for a given type. For example, even
 //      if you specify:
 //          1: bool enabled = true,
 //
 //       upon decoding, if the field enabled isn't present, the default value
 //       will be false.
 //
 //   2) not specifying optional won't actually make your field required,
 //      neither will specifying required make any field required. Upon decoding
 //      with serde, all values will be Default::default() and no error will be
 //      given.
 //
 //   3) the only way to detect wether a field was specified in the structure
 //      being deserialized is by making a field optional. This will result in
 //      a 'None' value for a Option<T> in rust. So the way we can give default
 //      values other then 'Default::default()' is by making a field optional,
 //      and then explicitly handle 'None' after deserialization.

struct RawRepoConfigs {
    1: map<string, RawCommitSyncConfig> (rust.type = "HashMap") commit_sync,
    2: RawCommonConfig common,
    3: map<string, RawRepoConfig> (rust.type = "HashMap") repos,
    4: map<string, RawStorageConfig> (rust.type = "HashMap") storage,
}

struct RawRepoConfig {
    // Most important - the unique ID of this Repo
    // Required - don't let the optional comment fool you, see notice above
    1: optional i32 repoid,

    // Persistent storage - contains location of metadata DB and name of
    // blobstore we're using. We reference the common storage config by name.
    // Required - don't let the optional comment fool you, see notice above
    2: optional string storage_config,

    // Local definitions of storage (override the global defined storage)
    3: optional map<string, RawStorageConfig> storage,

    // Repo is enabled for use
    4: optional bool enabled,

    // Repo is read-only (default false)
    5: optional bool readonly,

    // Define special bookmarks with parameters
    6: optional list<RawBookmarkConfig> bookmarks,
    7: optional i64 bookmarks_cache_ttl,

    // Define hook manager
    8: optional RawHookManagerParams hook_manager_params,

    // Define hook available for use on bookmarks
    9: optional list<RawHookConfig> hooks,

    // DB we're using for write-locking repos. This is separate from the rest
    // because it's the same one Mercurial uses, to make it easier to manage
    // repo locking for both from one tool.
    10: optional string write_lock_db_address,

    // This enables or disables verification for censored blobstores
    11: optional bool redaction,

    12: optional i64 generation_cache_size,
    13: optional string scuba_table,
    14: optional string scuba_table_hooks,
    15: optional i64 delay_mean,
    16: optional i64 delay_stddev,
    17: optional RawCacheWarmupConfig cache_warmup,
    18: optional RawPushParams push,
    19: optional RawPushrebaseParams pushrebase,
    20: optional RawLfsParams lfs,
    21: optional RawWireprotoLoggingConfig wireproto_logging,
    22: optional i64 hash_validation_percentage,
    23: optional string skiplist_index_blobstore_key,
    24: optional RawBundle2ReplayParams bundle2_replay_params,
    25: optional RawInfinitepushParams infinitepush,
    26: optional i64 list_keys_patterns_max,
    27: optional RawFilestoreParams filestore,
    28: optional i64 hook_max_file_size,
    29: optional string hipster_acl,
    31: optional RawSourceControlServiceParams source_control_service,
    30: optional RawSourceControlServiceMonitoring
                   source_control_service_monitoring,
    32: optional string name,
    // Types of derived data that are derived for this repo and are safe to use
    33: optional RawDerivedDataConfig derived_data_config,

    // Log Scuba samples to files. Largely only useful in tests.
    34: optional string scuba_local_path,
    35: optional string scuba_local_path_hooks,

    // Name of this repository in hgsql. This is used for syncing mechanisms
    // that interact directly with hgsql data, notably the hgsql repo lock
    36: optional string hgsql_name,

    // Name of this repository in hgsql for globalrevs. Required for syncing
    // globalrevs through the sync job.
    37: optional string hgsql_globalrevs_name,

    38: optional RawSegmentedChangelogConfig segmented_changelog_config,
    39: optional bool enforce_lfs_acl_check,
}

struct RawDerivedDataConfig {
  1: optional string scuba_table,
  2: optional set<string> derived_data_types,
  // Defaults to v1
  3: optional RawUnodeVersion raw_unode_version,
  4: optional i64 override_blame_filesize_limit,
}

union RawUnodeVersion {
  1: RawUnodeVersionV1 unode_version_v1,
  2: RawUnodeVersionV2 unode_version_v2,
}

struct RawUnodeVersionV1 {}

struct RawUnodeVersionV2 {}

struct RawBlobstoreDisabled {}
struct RawBlobstoreFilePath {
    1: string path,
}
struct RawBlobstoreManifold {
    1: string manifold_bucket,
    2: string manifold_prefix,
}
struct RawBlobstoreMysql {
    // 1: deleted
    // 2: deleted
    3: RawDbShardableRemote remote,
}
struct RawBlobstoreMultiplexed {
    1: optional string scuba_table,
    2: list<RawBlobstoreIdConfig> components,
    3: optional i64 scuba_sample_rate,
    4: optional i32 multiplex_id,
    5: optional RawDbConfig queue_db,
}
struct RawBlobstoreManifoldWithTtl {
    1: string manifold_bucket,
    2: string manifold_prefix,
    3: i64 ttl_secs,
}
struct RawBlobstoreLogging {
    1: optional string scuba_table,
    2: optional i64 scuba_sample_rate,
    3: RawBlobstoreConfig blobstore (rust.box),
}
struct RawBlobstorePack {
    1: RawBlobstoreConfig blobstore (rust.box),
}

// Configuration for a single blobstore. These are intended to be defined in a
// separate blobstore.toml config file, and then referenced by name from a
// per-server config. Names are only necessary for blobstores which are going
// to be used by a server. The id field identifies the blobstore as part of a
// multiplex, and need not be defined otherwise. However, once it has been set
// for a blobstore, it must remain unchanged.
union RawBlobstoreConfig {
    1: RawBlobstoreDisabled disabled,
    2: RawBlobstoreFilePath blob_files,
    // 3: deleted
    4: RawBlobstoreFilePath blob_sqlite,
    5: RawBlobstoreManifold manifold,
    6: RawBlobstoreMysql mysql,
    7: RawBlobstoreMultiplexed multiplexed,
    8: RawBlobstoreManifoldWithTtl manifold_with_ttl,
    9: RawBlobstoreLogging logging,
    10: RawBlobstorePack pack,
}

struct RawBlobstoreIdConfig {
    1: i64 blobstore_id,
    2: RawBlobstoreConfig blobstore,
}

struct RawDbLocal {
    1: string local_db_path,
}

struct RawDbRemote {
    1: string db_address,
    // 2: deleted
}

struct RawDbShardedRemote {
    1: string shard_map,
    2: i32 shard_num,
}

union RawDbShardableRemote {
    1: RawDbRemote unsharded,
    2: RawDbShardedRemote sharded,
}

union RawDbConfig {
    1: RawDbLocal local,
    2: RawDbRemote remote,
}

struct RawRemoteMetadataConfig {
    1: RawDbRemote primary,
    2: RawDbShardableRemote filenodes,
    3: RawDbRemote mutation,
}

union RawMetadataConfig {
    1: RawDbLocal local,
    2: RawRemoteMetadataConfig remote,
}

struct RawStorageConfig {
    // 1: deleted
    3: RawMetadataConfig metadata,
    2: RawBlobstoreConfig blobstore,
}

struct RawPushParams {
    1: optional bool pure_push_allowed,
    2: optional string commit_scribe_category,
}

struct RawPushrebaseParams {
    1: optional bool rewritedates,
    2: optional i64 recursion_limit,
    3: optional string commit_scribe_category,
    4: optional bool block_merges,
    5: optional bool forbid_p2_root_rebases,
    6: optional bool casefolding_check,
    7: optional bool emit_obsmarkers,
    8: optional bool assign_globalrevs,
    9: optional bool populate_git_mapping,
}

struct RawBookmarkConfig {
    // Either the regex or the name should be provided, not both
    1: optional string regex,
    2: optional string name,
    3: list<RawBookmarkHook> hooks,
    // Are non fastforward moves allowed for this bookmark
    4: bool only_fast_forward,
    // Only users matching this pattern (regex) will be allowed
    // to move this bookmark
    5: optional string allowed_users,
    // Whether or not to rewrite dates when processing pushrebase pushes
    6: optional bool rewrite_dates,
}

struct RawWhitelistEntry {
    1: optional string tier,
    2: optional string identity_data,
    3: optional string identity_type,
}

struct RawCommonConfig {
    1: optional list<RawWhitelistEntry> whitelist_entry,
    2: optional string loadlimiter_category,

    // Scuba table for logging redacted file access attempts
    3: optional string scuba_censored_table,
}

struct RawCacheWarmupConfig {
    1: string bookmark,
    2: optional i64 commit_limit,
    3: optional bool microwave_preload,
}

struct RawBookmarkHook {
    1: string hook_name,
}

struct RawHookManagerParams {
    /// Wether to disable the acl checker or not (intended for testing purposes)
    1: bool disable_acl_checker,
}

struct RawHookConfig {
    1: string name,
    4: optional string bypass_commit_string,
    5: optional string bypass_pushvar,
    6: optional map<string, string> (rust.type = "HashMap") config_strings,
    7: optional map<string, i32> (rust.type = "HashMap") config_ints,
}

struct RawLfsParams {
    1: optional i64 threshold,
    // What percentage of client host gets lfs pointers
    2: optional i32 rollout_percentage,
    // Whether to generate lfs blobs in hg sync job
    3: optional bool generate_lfs_blob_in_hg_sync_job,
    // If a hostname is in this smc tier then it will get
    // lfs pointers regardless of rollout_percentage
    4: optional string rollout_smc_tier,
}

struct RawBundle2ReplayParams {
    1: optional bool preserve_raw_bundle2,
}

struct RawInfinitepushParams {
    1: bool allow_writes,
    2: optional string namespace_pattern,
    3: optional bool hydrate_getbundle_response,
    4: optional bool populate_reverse_filler_queue,
    5: optional string commit_scribe_category,
}

struct RawFilestoreParams {
    1: i64 chunk_size,
    2: i32 concurrency,
}

struct RawCommitSyncSmallRepoConfig {
    1: i32 repoid,
    2: string default_action,
    3: optional string default_prefix,
    4: string bookmark_prefix,
    5: map<string, string> mapping,
    6: string direction,
}

struct RawCommitSyncConfig {
    1: i32 large_repo_id,
    2: list<string> common_pushrebase_bookmarks,
    3: list<RawCommitSyncSmallRepoConfig> small_repos,
    4: optional string version_name,
}

 struct RawWireprotoLoggingConfig {
     1: optional string scribe_category,
     2: optional string storage_config,
     3: optional i64 remote_arg_size_threshold,
     4: optional string local_path,
 }

struct RawSourceControlServiceParams {
    // Whether ordinary users can write through the source control service.
    1: bool permit_writes,

    // Whether service users can write through the source control service.
    2: bool permit_service_writes,

    // ACL to use for verifying a caller has write access on behalf of a service.
    3: optional string service_write_hipster_acl,

    // Map from service name to the restrictions that apply for that service.
    4: optional map<string, RawServiceWriteRestrictions> service_write_restrictions,
}

struct RawServiceWriteRestrictions {
    // The service is permitted to call these methods.
    1: set<string> permitted_methods,

    // The service is permitted to modify files with these path prefixes.
    2: optional set<string> permitted_path_prefixes,

    // The service is permitted to modify these bookmarks.
    3: optional set<string> permitted_bookmarks,

    // The service is permitted to modify bookmarks that match this regex.
    4: optional string permitted_bookmark_regex,
}

// Raw configuration for health monitoring of the
// source-control-as-a-service solutions
struct RawSourceControlServiceMonitoring {
    1: list<string> bookmarks_to_report_age,
}

struct RawSegmentedChangelogConfig {
    1: optional bool enabled,
}
