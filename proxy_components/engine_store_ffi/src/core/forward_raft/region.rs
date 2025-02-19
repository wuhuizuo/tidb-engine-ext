use proxy_ffi::fatal;

// Copyright 2022 TiKV Project Authors. Licensed under Apache-2.0.
use crate::core::{common::*, ProxyForwarder};

impl<T: Transport + 'static, ER: RaftEngine> ProxyForwarder<T, ER> {
    pub fn on_update_safe_ts(&self, region_id: u64, self_safe_ts: u64, leader_safe_ts: u64) {
        self.engine_store_server_helper.handle_safe_ts_update(
            region_id,
            self_safe_ts,
            leader_safe_ts,
        )
    }

    pub fn on_region_changed(&self, ob_region: &Region, e: RegionChangeEvent, r: StateRole) {
        self.engine_store_server_helper
            .maybe_jemalloc_register_alloc();
        self.engine_store_server_helper
            .directly_report_jemalloc_alloc();
        let region_id = ob_region.get_id();
        if e == RegionChangeEvent::Destroy {
            info!(
                "observe destroy";
                "region_id" => region_id,
                "store_id" => self.store_id,
            );
            self.engine_store_server_helper.handle_destroy(region_id);
            if self.packed_envs.engine_store_cfg.enable_fast_add_peer {
                self.get_cached_manager()
                    .remove_cached_region_info(region_id);
            }
        } else if e == RegionChangeEvent::Create {
            // This could happen when restore.
            let f = |info: MapEntry<u64, Arc<CachedRegionInfo>>| match info {
                MapEntry::Occupied(_) => {}
                MapEntry::Vacant(v) => {
                    info!("{}{}:{}, peer created(region event)",
                        self.store_id, region_id, 0;
                        "region_id" => region_id,
                        "role" => ?r,
                    );
                    let c = CachedRegionInfo::default();
                    c.replicated_or_created.store(true, Ordering::SeqCst);
                    c.inited_or_fallback
                        .store(self.is_initialized(region_id), Ordering::SeqCst);
                    v.insert(Arc::new(c));
                }
            };
            if self
                .get_cached_manager()
                .access_cached_region_info_mut(region_id, f)
                .is_err()
            {
                fatal!(
                    "on_region_changed could not found region info region_id={}",
                    region_id
                );
            }
        }
    }

    #[allow(clippy::match_like_matches_macro)]
    pub fn pre_persist(
        &self,
        ob_region: &Region,
        is_finished: bool,
        cmd: Option<&RaftCmdRequest>,
    ) -> bool {
        let region_id = ob_region.get_id();
        let should_persist = if is_finished {
            true
        } else {
            let cmd = cmd.unwrap();
            if cmd.has_admin_request() {
                match cmd.get_admin_request().get_cmd_type() {
                    // Merge needs to get the latest apply index.
                    AdminCmdType::CommitMerge | AdminCmdType::RollbackMerge => true,
                    _ => false,
                }
            } else {
                false
            }
        };
        if should_persist {
            debug!(
            "observe pre_persist, persist";
            "region_id" => region_id,
            "store_id" => self.store_id,
            );
        } else {
            debug!(
            "observe pre_persist";
            "region_id" => region_id,
            "store_id" => self.store_id,
            "is_finished" => is_finished,
            );
        };
        should_persist
    }

    pub fn pre_write_apply_state(&self, _ob_region: &Region) -> bool {
        fail::fail_point!("on_pre_write_apply_state", |_| {
            // Some test need persist apply state for Leader logic,
            // including fast add peer.
            true
        });
        false
    }

    pub fn on_role_change(&self, ob_region: &Region, r: &RoleChange) {
        self.engine_store_server_helper
            .maybe_jemalloc_register_alloc();
        self.engine_store_server_helper
            .directly_report_jemalloc_alloc();
        let region_id = ob_region.get_id();
        let is_replicated = !r.initialized;
        let is_fap_enabled = if let Some(b) = self.engine.proxy_ext.config_set.as_ref() {
            b.engine_store.enable_fast_add_peer
        } else {
            false
        };
        let f = |info: MapEntry<u64, Arc<CachedRegionInfo>>| match info {
            MapEntry::Occupied(mut o) => {
                // Note the region info may be registered by maybe_fast_path_tick
                info!("{}{}:{} {}, peer changed",
                    if is_fap_enabled {"fast path: ongoing "} else {" "},
                    self.store_id, region_id, 0;
                    "region_id" => region_id,
                    "leader_id" => r.leader_id,
                    "role" => ?r.state,
                    "is_replicated" => is_replicated,
                );
                if is_replicated {
                    o.get_mut()
                        .replicated_or_created
                        .store(true, Ordering::SeqCst);
                }
            }
            MapEntry::Vacant(v) => {
                info!("{}{}:{} {}, peer created(role event)",
                    if is_fap_enabled {"fast path: ongoing "} else {" "},
                    self.store_id, region_id, r.peer_id;
                    "region_id" => region_id,
                    "leader_id" => r.leader_id,
                    "role" => ?r.state,
                    "is_replicated" => is_replicated,
                );
                let c = CachedRegionInfo::default();
                c.replicated_or_created.store(true, Ordering::SeqCst);
                c.inited_or_fallback.store(r.initialized, Ordering::SeqCst);
                v.insert(Arc::new(c));
            }
        };
        if self
            .get_cached_manager()
            .access_cached_region_info_mut(region_id, f)
            .is_err()
        {
            fatal!(
                "on_role_change could not found region info region_id={}",
                region_id
            );
        }
    }
}
