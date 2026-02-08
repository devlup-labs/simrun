/**
 * Serializes architecture canvas data to IR (Intermediate Representation) format
 * that the compiler expects
 */

import { SimulationExport } from '@/types/simulation';

export interface IRHeader {
  ir_version: string;
  engine_version: string;
  seed: number;
  time_unit: string;
}

export interface ComponentConfig {
  [key: string]: number | string | boolean | undefined;
}

export interface Component {
  id: number;
  type: string;
  config: ComponentConfig;
}

export interface Link {
  id: number;
  from: number;
  to: number;
  config: ComponentConfig;
}

export interface Context {
  components: Component[];
  links: Link[];
}

export interface IR {
  header: IRHeader;
  context: Context;
}

/**
 * Convert SimulationExport from canvas to IR format expected by compiler
 */
export function serializeToIR(exportData: SimulationExport): IR {
  // Build header
  const header: IRHeader = {
    ir_version: '1.0',
    engine_version: 'simrun-0.1',
    seed: 42,
    time_unit: 'milliseconds',
  };

  // Build components - convert id from string to positive number
  const components: Component[] = exportData.components.map((comp, index) => {
    // Generate a positive numeric ID from the string ID
    // Use a simple hash: just take last 8 digits if available, or use index
    let numericId = 0;
    
    // Try to extract digits and use the last portion (more unique per component)
    const digitsOnly = comp.id.replace(/\D/g, '');
    if (digitsOnly && digitsOnly.length > 0) {
      // Take the last 8 characters to get a manageable number
      const lastDigits = digitsOnly.slice(-8);
      numericId = parseInt(lastDigits, 10);
    }
    
    // If still invalid or 0, use index + 1 (always > 0)
    if (!numericId || numericId <= 0 || Number.isNaN(numericId)) {
      numericId = index + 1;
      console.warn(`[irSerializer] Component "${comp.id}" → ID generated from index: ${numericId}`);
    } else {
      console.log(`[irSerializer] Component "${comp.id}" → numeric ID: ${numericId}`);
    }
    
    // Build config based on component type
    const config: ComponentConfig = {};
    
    if (comp.parameters) {
      const params = comp.parameters as any;
      
      switch (comp.type) {
        case 'api':
        case 'api/service':
          config.dist_latency = 'lognormal';
          config.base_median_latency = params.processing_latency_ms || 30;
          config.base_variance_latency = 0.8;
          config.max_concurrency = params.max_concurrency || 100;
          config.queue_capacity = 300;
          break;
          
        case 'database':
          config.max_iops = 3000;
          config.base_seek_time = params.base_latency_ms || 4;
          config.max_concurrency = params.max_concurrency || 1000;
          config.queue_capacity = 3000;
          break;
          
        case 'cache':
          config.cache_hit_probability = params.hit_rate || 0.7;
          config.cache_hit_latency = 0.3;
          config.cache_miss_latency = 0.1;
          break;
          
        default:
          // Generic parameters
          Object.assign(config, params);
      }
    }
    
    return {
      id: numericId,
      type: comp.type,
      config,
    };
  });

  // Build links - convert ids to numbers
  const links: Link[] = exportData.links.map((link, index) => {
    // Extract numeric IDs from source and target
    let fromId = 0;
    let toId = 0;
    
    // Helper function to extract numeric ID
    const extractNumericId = (nodeId: string): number => {
      const digitsOnly = nodeId.replace(/\D/g, '');
      if (digitsOnly && digitsOnly.length > 0) {
        const lastDigits = digitsOnly.slice(-8);
        return parseInt(lastDigits, 10);
      }
      return 0;
    };
    
    fromId = extractNumericId(link.source);
    toId = extractNumericId(link.target);
    
    // Fallback to component 1 if extraction failed
    if (!fromId || fromId <= 0 || Number.isNaN(fromId)) {
      console.warn(`[irSerializer] Link source "${link.source}" → using default: 1`);
      fromId = 1;
    }
    
    if (!toId || toId <= 0 || Number.isNaN(toId)) {
      console.warn(`[irSerializer] Link target "${link.target}" → using default: 1`);
      toId = 1;
    }
    
    console.log(`[irSerializer] Link: "${link.source}" → "${link.target}" | IDs: ${fromId} → ${toId}`);
    
    const config: ComponentConfig = {};
    if (link.parameters) {
      const params = link.parameters as any;
      config.base_latency = params.latency_ms || 1;
      config.base_bandwidth_mbps = params.bandwidth_limit || 1000;
      config.packet_size_bytes = 1024;
    }
    
    return {
      id: index + 100, // Start link IDs from 100 to avoid collision with components
      from: fromId,
      to: toId,
      config,
    };
  });

  return {
    header,
    context: {
      components,
      links,
    },
  };
}

/**
 * Wraps IR in the format expected by the compiler server
 */
export function wrapForCompiler(ir: IR): { project: IR } {
  return { project: ir };
}
