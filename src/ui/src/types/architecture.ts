// Architecture component types and profiles
export type ComponentCategory = 'database' | 'cache' | 'api' | 'network';

export type DatabaseProfile = 'mongodb' | 'mysql' | 'postgresql';
export type CacheProfile = 'redis' | 'memcached';
export type ApiProfile = 'rest' | 'grpc';
export type NetworkProfile = 'standard' | 'high-latency' | 'lossy';

export type ComponentProfile = DatabaseProfile | CacheProfile | ApiProfile | NetworkProfile;

export interface ComponentDefinition {
  id: string;
  category: ComponentCategory;
  profile: ComponentProfile;
  name: string;
  description: string;
  icon: string;
}

export interface ArchitectureNode {
  id: string;
  type: ComponentCategory;
  profile: ComponentProfile;
  position: { x: number; y: number };
  data: {
    label: string;
    profile: ComponentProfile;
    category: ComponentCategory;
    parameters?: Record<string, unknown>;
  };
}

export interface ArchitectureEdge {
  id: string;
  source: string;
  target: string;
  type?: string;
  data?: {
    linkType?: NetworkProfile;
  };
}

export interface ArchitectureGraph {
  nodes: ArchitectureNode[];
  edges: ArchitectureEdge[];
  metadata?: {
    name?: string;
    version?: string;
    createdAt?: string;
    updatedAt?: string;
  };
}

// Component palette definitions
export const COMPONENT_DEFINITIONS: ComponentDefinition[] = [
  // Databases
  {
    id: 'mongodb',
    category: 'database',
    profile: 'mongodb',
    name: 'MongoDB',
    description: 'Document-oriented NoSQL database',
    icon: 'Database',
  },
  {
    id: 'mysql',
    category: 'database',
    profile: 'mysql',
    name: 'MySQL',
    description: 'Relational database management system',
    icon: 'Database',
  },
  {
    id: 'postgresql',
    category: 'database',
    profile: 'postgresql',
    name: 'PostgreSQL',
    description: 'Advanced open-source relational database',
    icon: 'Database',
  },
  // Caches
  {
    id: 'redis',
    category: 'cache',
    profile: 'redis',
    name: 'Redis',
    description: 'In-memory data structure store',
    icon: 'Zap',
  },
  {
    id: 'memcached',
    category: 'cache',
    profile: 'memcached',
    name: 'Memcached',
    description: 'High-performance distributed memory cache',
    icon: 'Zap',
  },
  // API Services
  {
    id: 'rest',
    category: 'api',
    profile: 'rest',
    name: 'REST API Service',
    description: 'RESTful HTTP API endpoint',
    icon: 'Globe',
  },
  {
    id: 'grpc',
    category: 'api',
    profile: 'grpc',
    name: 'gRPC Service',
    description: 'High-performance RPC framework',
    icon: 'Radio',
  },
  // Network Links
  {
    id: 'standard',
    category: 'network',
    profile: 'standard',
    name: 'Standard Network Link',
    description: 'Normal network connection',
    icon: 'Link',
  },
  {
    id: 'high-latency',
    category: 'network',
    profile: 'high-latency',
    name: 'High-Latency Link',
    description: 'Slow network connection',
    icon: 'Clock',
  },
  {
    id: 'lossy',
    category: 'network',
    profile: 'lossy',
    name: 'Lossy Network Link',
    description: 'Unreliable network with packet loss',
    icon: 'AlertTriangle',
  },
];

export const CATEGORY_LABELS: Record<ComponentCategory, string> = {
  database: 'Database',
  cache: 'Cache',
  api: 'API / Service',
  network: 'Network',
};
