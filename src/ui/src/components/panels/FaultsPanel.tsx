import { Plus, Trash2, AlertTriangle } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Slider } from '@/components/ui/slider';
import { ScrollArea } from '@/components/ui/scroll-area';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { HelpCircle } from 'lucide-react';
import { useSimulationStore } from '@/store/simulationStore';
import { useArchitectureStore } from '@/store/architectureStore';
import { Fault, FaultType, FaultMode } from '@/types/simulation';

const FAULT_TYPE_LABELS: Record<FaultType, string> = {
  disk_failure: 'Disk Failure',
  latency_spike: 'Latency Spike',
  node_crash: 'Node Crash',
};

const FAULT_MODE_LABELS: Record<FaultMode, string> = {
  probability: 'Probability-based',
  scheduled: 'Time-scheduled',
};

export const FaultsPanel = () => {
  const { faults, addFault, updateFault, deleteFault } = useSimulationStore();
  const { nodes, edges } = useArchitectureStore();

  const handleAddFault = () => {
    const newFault: Fault = {
      id: `fault_${Date.now()}`,
      targetId: nodes[0]?.id || '',
      targetType: 'node',
      faultType: 'latency_spike',
      mode: 'probability',
      probability: 0.05,
      duration_ms: 1000,
    };
    addFault(newFault);
  };

  const getTargetLabel = (fault: Fault) => {
    if (fault.targetType === 'node') {
      const node = nodes.find((n) => n.id === fault.targetId);
      return (node?.data.label as string) || fault.targetId;
    } else {
      const edge = edges.find((e) => e.id === fault.targetId);
      if (edge) {
        const source = nodes.find((n) => n.id === edge.source);
        const target = nodes.find((n) => n.id === edge.target);
        return `${source?.data.label || edge.source} → ${target?.data.label || edge.target}`;
      }
      return fault.targetId;
    }
  };

  return (
    <div className="h-full flex flex-col bg-sidebar">
      <div className="px-4 py-3 border-b border-sidebar-border flex items-center justify-between">
        <div className="flex items-center gap-2">
          <AlertTriangle className="w-4 h-4 text-destructive" />
          <h3 className="text-sm font-semibold text-foreground">Fault Injection</h3>
        </div>
        <Button size="sm" variant="outline" onClick={handleAddFault} className="h-7 px-2">
          <Plus className="w-4 h-4" />
        </Button>
      </div>

      <ScrollArea className="flex-1">
        <div className="p-4 space-y-4">
          {faults.length === 0 ? (
            <p className="text-sm text-muted-foreground text-center py-8">
              No faults defined. Add faults to simulate failures.
            </p>
          ) : (
            faults.map((fault, index) => (
              <div key={fault.id} className="p-3 border border-border rounded-lg space-y-4">
                <div className="flex items-center justify-between">
                  <span className="text-xs font-medium">Fault {index + 1}</span>
                  <Button
                    size="icon"
                    variant="ghost"
                    className="h-6 w-6"
                    onClick={() => deleteFault(fault.id)}
                  >
                    <Trash2 className="w-3 h-3 text-destructive" />
                  </Button>
                </div>

                {/* Target Type */}
                <div className="space-y-2">
                  <Label className="text-xs text-muted-foreground uppercase tracking-wider">Target Type</Label>
                  <Select
                    value={fault.targetType}
                    onValueChange={(v) =>
                      updateFault(fault.id, {
                        targetType: v as 'node' | 'edge',
                        targetId: v === 'node' ? nodes[0]?.id || '' : edges[0]?.id || '',
                      })
                    }
                  >
                    <SelectTrigger className="bg-input border-border h-8">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="node">Node</SelectItem>
                      <SelectItem value="edge">Edge (Link)</SelectItem>
                    </SelectContent>
                  </Select>
                </div>

                {/* Target */}
                <div className="space-y-2">
                  <Label className="text-xs text-muted-foreground uppercase tracking-wider">Target</Label>
                  <Select
                    value={fault.targetId}
                    onValueChange={(v) => updateFault(fault.id, { targetId: v })}
                  >
                    <SelectTrigger className="bg-input border-border h-8">
                      <SelectValue placeholder="Select target" />
                    </SelectTrigger>
                    <SelectContent>
                      {fault.targetType === 'node'
                        ? nodes.map((node) => (
                            <SelectItem key={node.id} value={node.id}>
                              {node.data.label as string}
                            </SelectItem>
                          ))
                        : edges.map((edge) => {
                            const source = nodes.find((n) => n.id === edge.source);
                            const target = nodes.find((n) => n.id === edge.target);
                            return (
                              <SelectItem key={edge.id} value={edge.id}>
                                {(source?.data.label as string) || edge.source} → {(target?.data.label as string) || edge.target}
                              </SelectItem>
                            );
                          })}
                    </SelectContent>
                  </Select>
                </div>

                {/* Fault Type */}
                <div className="space-y-2">
                  <Label className="text-xs text-muted-foreground uppercase tracking-wider">Fault Type</Label>
                  <Select
                    value={fault.faultType}
                    onValueChange={(v) => updateFault(fault.id, { faultType: v as FaultType })}
                  >
                    <SelectTrigger className="bg-input border-border h-8">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent>
                      {(Object.keys(FAULT_TYPE_LABELS) as FaultType[]).map((type) => (
                        <SelectItem key={type} value={type}>
                          {FAULT_TYPE_LABELS[type]}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>

                {/* Mode */}
                <div className="space-y-2">
                  <div className="flex items-center gap-2">
                    <Label className="text-xs text-muted-foreground uppercase tracking-wider">Mode</Label>
                    <Tooltip>
                      <TooltipTrigger>
                        <HelpCircle className="w-3 h-3 text-muted-foreground" />
                      </TooltipTrigger>
                      <TooltipContent>
                        <p className="max-w-xs text-xs">
                          Probability: random chance each tick. Scheduled: happens at specific time.
                        </p>
                      </TooltipContent>
                    </Tooltip>
                  </div>
                  <Select
                    value={fault.mode}
                    onValueChange={(v) => updateFault(fault.id, { mode: v as FaultMode })}
                  >
                    <SelectTrigger className="bg-input border-border h-8">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent>
                      {(Object.keys(FAULT_MODE_LABELS) as FaultMode[]).map((mode) => (
                        <SelectItem key={mode} value={mode}>
                          {FAULT_MODE_LABELS[mode]}
                        </SelectItem>
                      ))}
                    </SelectContent>
                  </Select>
                </div>

                {/* Mode-specific parameters */}
                {fault.mode === 'probability' && (
                  <div className="space-y-2">
                    <div className="flex items-center justify-between">
                      <Label className="text-xs text-muted-foreground uppercase tracking-wider">Probability</Label>
                      <span className="text-xs font-mono text-muted-foreground">
                        {((fault.probability || 0) * 100).toFixed(1)}%
                      </span>
                    </div>
                    <Slider
                      value={[(fault.probability || 0) * 100]}
                      onValueChange={([v]) => updateFault(fault.id, { probability: v / 100 })}
                      min={0}
                      max={100}
                      step={0.5}
                      className="py-2"
                    />
                  </div>
                )}

                {fault.mode === 'scheduled' && (
                  <div className="space-y-2">
                    <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                      Scheduled Time (ms)
                    </Label>
                    <Input
                      type="number"
                      min={0}
                      value={fault.scheduled_time_ms || 0}
                      onChange={(e) =>
                        updateFault(fault.id, { scheduled_time_ms: parseInt(e.target.value) || 0 })
                      }
                      className="bg-input border-border h-8"
                    />
                  </div>
                )}

                {/* Duration */}
                <div className="space-y-2">
                  <Label className="text-xs text-muted-foreground uppercase tracking-wider">Duration (ms)</Label>
                  <Input
                    type="number"
                    min={0}
                    value={fault.duration_ms || 0}
                    onChange={(e) =>
                      updateFault(fault.id, { duration_ms: parseInt(e.target.value) || 0 })
                    }
                    className="bg-input border-border h-8"
                  />
                </div>
              </div>
            ))
          )}
        </div>
      </ScrollArea>
    </div>
  );
};
