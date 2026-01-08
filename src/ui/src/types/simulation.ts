// Simulation configuration types

// Component parameter types based on category
export interface DatabaseParameters {
  max_concurrency: number;
  base_latency_ms: number;
  disk_fail_prob: number;
  read_write_ratio?: number;
}

export interface CacheParameters {
  max_concurrency: number;
  hit_rate: number;
  eviction_policy: 'lru' | 'lfu' | 'fifo' | 'random';
}

export interface ApiParameters {
  max_concurrency: number;
  processing_latency_ms: number;
  timeout_ms: number;
  retry_count: number;
}

export interface NetworkParameters {
  latency_ms: number;
  loss_prob: number;
  bandwidth_limit?: number;
}

export type ComponentParameters = DatabaseParameters | CacheParameters | ApiParameters | NetworkParameters;

// Default parameters per profile
export const DEFAULT_DATABASE_PARAMS: DatabaseParameters = {
  max_concurrency: 100,
  base_latency_ms: 10,
  disk_fail_prob: 0.001,
  read_write_ratio: 0.8,
};

export const DEFAULT_CACHE_PARAMS: CacheParameters = {
  max_concurrency: 1000,
  hit_rate: 0.85,
  eviction_policy: 'lru',
};

export const DEFAULT_API_PARAMS: ApiParameters = {
  max_concurrency: 200,
  processing_latency_ms: 50,
  timeout_ms: 5000,
  retry_count: 3,
};

export const DEFAULT_NETWORK_PARAMS: NetworkParameters = {
  latency_ms: 5,
  loss_prob: 0,
  bandwidth_limit: undefined,
};

// Route definition
export interface Route {
  id: string;
  name: string;
  entryNodeId: string;
  path: string[]; // ordered node IDs
  weight: number; // percentage
}

// Workload configuration
export type WorkloadType = 'steady' | 'bursty' | 'ramp-up';

// Distribution types for traffic patterns
export type DistributionType = 'constant' | 'linear' | 'sinusoidal' | 'poisson' | 'exponential' | 'normal';

export interface DistributionParams {
  // Poisson
  lambda?: number;
  // Normal (Gaussian)
  mean?: number;
  variance?: number;
  // Linear
  slope?: number;
  // Sinusoidal
  amplitude?: number;
  period_ms?: number;
  // Exponential
  decay_rate?: number;
}

export interface WorkloadSpike {
  id: string;
  time_ms: number;
  rps: number;
  duration_ms: number;
}

export interface WorkloadConfig {
  type: WorkloadType;
  base_rps: number;
  duration_ms: number;
  spikes: WorkloadSpike[];
  distribution: DistributionType;
  distribution_params: DistributionParams;
}

export const DEFAULT_WORKLOAD: WorkloadConfig = {
  type: 'steady',
  base_rps: 100,
  duration_ms: 60000,
  spikes: [],
  distribution: 'constant',
  distribution_params: {},
};

// Fault injection
export type FaultType = 'disk_failure' | 'latency_spike' | 'node_crash';
export type FaultMode = 'probability' | 'scheduled';

export interface Fault {
  id: string;
  targetId: string; // node or edge ID
  targetType: 'node' | 'edge';
  faultType: FaultType;
  mode: FaultMode;
  probability?: number; // for probability mode (0-1)
  scheduled_time_ms?: number; // for scheduled mode
  duration_ms?: number;
}

// Extended export format
export interface SimulationExport {
  components: Array<{
    id: string;
    type: string;
    profile: string;
    position: { x: number; y: number };
    label: string;
    parameters: ComponentParameters;
  }>;
  links: Array<{
    id: string;
    source: string;
    target: string;
    parameters: NetworkParameters;
  }>;
  routes: Route[];
  workload: WorkloadConfig;
  faults: Fault[];
  metadata?: {
    name?: string;
    version?: string;
    createdAt?: string;
  };
}
