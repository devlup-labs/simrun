import { create } from 'zustand';
import { Route, WorkloadConfig, Fault, DEFAULT_WORKLOAD } from '@/types/simulation';

interface SimulationState {
  routes: Route[];
  workload: WorkloadConfig;
  faults: Fault[];

  // Route actions
  addRoute: (route: Route) => void;
  updateRoute: (id: string, updates: Partial<Route>) => void;
  deleteRoute: (id: string) => void;

  // Workload actions
  setWorkload: (workload: WorkloadConfig) => void;

  // Fault actions
  addFault: (fault: Fault) => void;
  updateFault: (id: string, updates: Partial<Fault>) => void;
  deleteFault: (id: string) => void;

  // Reset
  clearAll: () => void;
}

const generateId = () => `${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

export const useSimulationStore = create<SimulationState>((set, get) => ({
  routes: [],
  workload: DEFAULT_WORKLOAD,
  faults: [],

  addRoute: (route) => {
    set({ routes: [...get().routes, { ...route, id: route.id || generateId() }] });
  },

  updateRoute: (id, updates) => {
    set({
      routes: get().routes.map((r) => (r.id === id ? { ...r, ...updates } : r)),
    });
  },

  deleteRoute: (id) => {
    set({ routes: get().routes.filter((r) => r.id !== id) });
  },

  setWorkload: (workload) => {
    set({ workload });
  },

  addFault: (fault) => {
    set({ faults: [...get().faults, { ...fault, id: fault.id || generateId() }] });
  },

  updateFault: (id, updates) => {
    set({
      faults: get().faults.map((f) => (f.id === id ? { ...f, ...updates } : f)),
    });
  },

  deleteFault: (id) => {
    set({ faults: get().faults.filter((f) => f.id !== id) });
  },

  clearAll: () => {
    set({ routes: [], workload: DEFAULT_WORKLOAD, faults: [] });
  },
}));
