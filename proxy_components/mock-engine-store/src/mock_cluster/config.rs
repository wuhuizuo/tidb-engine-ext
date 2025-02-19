// Copyright 2022 TiKV Project Authors. Licensed under Apache-2.0.

use std::{
    ops::{Deref, DerefMut},
    sync::{atomic::AtomicBool, Arc},
};

use tikv::config::TikvConfig;

use super::common::*;

#[derive(Clone, Default)]
pub struct MockConfig {
    pub panic_when_flush_no_found: Arc<AtomicBool>,
    /// Whether our mock server should compat new proxy.
    pub proxy_compat: bool,
}

#[derive(Clone)]
pub struct MixedClusterConfig {
    pub tikv: TikvConfig,
    pub prefer_mem: bool,
    pub proxy_cfg: ProxyConfig,
    pub mock_cfg: MockConfig,
}

impl Deref for MixedClusterConfig {
    type Target = TikvConfig;
    #[inline]
    fn deref(&self) -> &TikvConfig {
        &self.tikv
    }
}

impl DerefMut for MixedClusterConfig {
    #[inline]
    fn deref_mut(&mut self) -> &mut TikvConfig {
        &mut self.tikv
    }
}
