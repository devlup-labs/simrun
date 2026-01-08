import { useState } from 'react';
import { Plus, Trash2, Activity, Zap, HelpCircle, BarChart3, TrendingUp, Waves, Sparkles, TrendingDown, Dice5, Info } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Separator } from '@/components/ui/separator';
import { ScrollArea } from '@/components/ui/scroll-area';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { Slider } from '@/components/ui/slider';
import { Badge } from '@/components/ui/badge';
import { useSimulationStore } from '@/store/simulationStore';
import { WorkloadConfig, WorkloadSpike, WorkloadType, DistributionType, DistributionParams } from '@/types/simulation';
import { cn } from '@/lib/utils';

const DISTRIBUTION_OPTIONS: Array<{
  value: DistributionType;
  label: string;
  icon: React.ReactNode;
  description: string;
}> = [
  { value: 'constant', label: 'Constant', icon: <BarChart3 className="w-4 h-4" />, description: 'Steady, uniform load' },
  { value: 'linear', label: 'Linear', icon: <TrendingUp className="w-4 h-4" />, description: 'Gradually increasing/decreasing' },
  { value: 'sinusoidal', label: 'Sinusoidal', icon: <Waves className="w-4 h-4" />, description: 'Wave pattern, cyclic load' },
  { value: 'poisson', label: 'Poisson', icon: <Sparkles className="w-4 h-4" />, description: 'Random arrival, realistic traffic' },
  { value: 'exponential', label: 'Exponential', icon: <TrendingDown className="w-4 h-4" />, description: 'Burst then decay' },
  { value: 'normal', label: 'Normal (Gaussian)', icon: <Dice5 className="w-4 h-4" />, description: 'Bell curve distribution' },
];

export const WorkloadPanel = () => {
  const { workload, setWorkload } = useSimulationStore();
  const [hoveredDistribution, setHoveredDistribution] = useState<DistributionType | null>(null);

  const handleChange = <K extends keyof WorkloadConfig>(key: K, value: WorkloadConfig[K]) => {
    setWorkload({ ...workload, [key]: value });
  };

  const handleDistributionParamChange = <K extends keyof DistributionParams>(key: K, value: DistributionParams[K]) => {
    setWorkload({
      ...workload,
      distribution_params: { ...workload.distribution_params, [key]: value },
    });
  };

  const handleAddSpike = () => {
    const newSpike: WorkloadSpike = {
      id: `spike_${Date.now()}`,
      time_ms: 30000,
      rps: workload.base_rps * 2,
      duration_ms: 5000,
    };
    setWorkload({ ...workload, spikes: [...workload.spikes, newSpike] });
  };

  const handleUpdateSpike = (id: string, updates: Partial<WorkloadSpike>) => {
    setWorkload({
      ...workload,
      spikes: workload.spikes.map((s) => (s.id === id ? { ...s, ...updates } : s)),
    });
  };

  const handleDeleteSpike = (id: string) => {
    setWorkload({
      ...workload,
      spikes: workload.spikes.filter((s) => s.id !== id),
    });
  };

  const renderDistributionParams = () => {
    const params = workload.distribution_params;

    switch (workload.distribution) {
      case 'constant':
        return (
          <div className="p-3 bg-primary/5 border border-primary/20 rounded-xl">
            <div className="flex items-center gap-2 text-xs text-muted-foreground">
              <Info className="w-3.5 h-3.5 text-primary" />
              <span>Maintains steady RPS throughout the simulation</span>
            </div>
          </div>
        );

      case 'poisson':
        return (
          <div className="space-y-4">
            <div className="space-y-3">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                    Lambda (λ)
                  </Label>
                  <Tooltip>
                    <TooltipTrigger>
                      <HelpCircle className="w-3 h-3 text-muted-foreground" />
                    </TooltipTrigger>
                    <TooltipContent>
                      <p className="max-w-xs text-xs">Average rate parameter. Controls the average number of events per time unit.</p>
                    </TooltipContent>
                  </Tooltip>
                </div>
                <Badge variant="secondary" className="text-xs font-mono">
                  {params.lambda ?? workload.base_rps}
                </Badge>
              </div>
              <div className="flex gap-3 items-center">
                <Slider
                  value={[params.lambda ?? workload.base_rps]}
                  onValueChange={([v]) => handleDistributionParamChange('lambda', v)}
                  min={0.1}
                  max={100}
                  step={0.1}
                  className="flex-1"
                />
                <Input
                  type="number"
                  min={0.1}
                  step={0.1}
                  value={params.lambda ?? workload.base_rps}
                  onChange={(e) => handleDistributionParamChange('lambda', parseFloat(e.target.value) || workload.base_rps)}
                  className="w-20 h-8 bg-input border-border text-xs"
                />
              </div>
            </div>
          </div>
        );

      case 'normal':
        const mean = params.mean ?? workload.base_rps;
        const variance = params.variance ?? 1;
        const showVarianceWarning = variance > mean;

        return (
          <div className="space-y-4">
            {/* Mean */}
            <div className="space-y-3">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <Label className="text-xs uppercase tracking-wider" style={{ color: 'hsl(var(--database))' }}>
                    Mean (μ)
                  </Label>
                  <span className="text-[10px] text-muted-foreground">RPS</span>
                </div>
              </div>
              <p className="text-[11px] text-muted-foreground -mt-1">Average Request Rate</p>
              <Input
                type="number"
                min={1}
                value={mean}
                onChange={(e) => handleDistributionParamChange('mean', parseFloat(e.target.value) || workload.base_rps)}
                className={cn(
                  "bg-input border-border h-10 text-lg font-mono",
                  "focus:border-database focus:ring-database/20"
                )}
              />
            </div>

            {/* Variance */}
            <div className="space-y-3">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <Label className="text-xs uppercase tracking-wider" style={{ color: 'hsl(var(--network))' }}>
                    Variance (σ²)
                  </Label>
                  <span className="text-[10px] text-muted-foreground">RPS²</span>
                  <Tooltip>
                    <TooltipTrigger>
                      <HelpCircle className="w-3 h-3 text-muted-foreground" />
                    </TooltipTrigger>
                    <TooltipContent>
                      <p className="max-w-xs text-xs">Spread of request rate fluctuation. Lower values = more consistent traffic.</p>
                    </TooltipContent>
                  </Tooltip>
                </div>
              </div>
              <p className="text-[11px] text-muted-foreground -mt-1">Traffic Variance</p>
              <Input
                type="number"
                min={0.1}
                step={0.1}
                value={variance}
                onChange={(e) => handleDistributionParamChange('variance', parseFloat(e.target.value) || 1)}
                className={cn(
                  "bg-input border-border h-10 text-lg font-mono",
                  "focus:border-network focus:ring-network/20",
                  showVarianceWarning && "border-cache"
                )}
              />
              <p className="text-[10px] text-muted-foreground">
                e.g., 0.5 means low variance, 2.0 means high variance
              </p>
              {showVarianceWarning && (
                <p className="text-[10px] text-cache flex items-center gap-1">
                  <Info className="w-3 h-3" />
                  High variance relative to mean may produce negative values
                </p>
              )}
            </div>
          </div>
        );

      case 'linear':
        return (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                  Slope
                </Label>
                <Tooltip>
                  <TooltipTrigger>
                    <HelpCircle className="w-3 h-3 text-muted-foreground" />
                  </TooltipTrigger>
                  <TooltipContent>
                    <p className="max-w-xs text-xs">Rate of change per second. Positive = increasing, Negative = decreasing.</p>
                  </TooltipContent>
                </Tooltip>
              </div>
              <Badge variant="secondary" className="text-xs font-mono">
                {params.slope ?? 1} RPS/s
              </Badge>
            </div>
            <Input
              type="number"
              step={0.1}
              value={params.slope ?? 1}
              onChange={(e) => handleDistributionParamChange('slope', parseFloat(e.target.value) || 1)}
              className="bg-input border-border"
            />
          </div>
        );

      case 'sinusoidal':
        return (
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-2">
              <div className="flex items-center gap-2">
                <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                  Amplitude
                </Label>
                <Tooltip>
                  <TooltipTrigger>
                    <HelpCircle className="w-3 h-3 text-muted-foreground" />
                  </TooltipTrigger>
                  <TooltipContent>
                    <p className="max-w-xs text-xs">Peak deviation from base RPS</p>
                  </TooltipContent>
                </Tooltip>
              </div>
              <Input
                type="number"
                min={1}
                value={params.amplitude ?? Math.floor(workload.base_rps / 2)}
                onChange={(e) => handleDistributionParamChange('amplitude', parseInt(e.target.value) || 50)}
                className="bg-input border-border"
              />
            </div>
            <div className="space-y-2">
              <div className="flex items-center gap-2">
                <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                  Period (ms)
                </Label>
                <Tooltip>
                  <TooltipTrigger>
                    <HelpCircle className="w-3 h-3 text-muted-foreground" />
                  </TooltipTrigger>
                  <TooltipContent>
                    <p className="max-w-xs text-xs">Duration of one complete wave cycle</p>
                  </TooltipContent>
                </Tooltip>
              </div>
              <Input
                type="number"
                min={1000}
                step={1000}
                value={params.period_ms ?? 10000}
                onChange={(e) => handleDistributionParamChange('period_ms', parseInt(e.target.value) || 10000)}
                className="bg-input border-border"
              />
            </div>
          </div>
        );

      case 'exponential':
        return (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                  Decay Rate
                </Label>
                <Tooltip>
                  <TooltipTrigger>
                    <HelpCircle className="w-3 h-3 text-muted-foreground" />
                  </TooltipTrigger>
                  <TooltipContent>
                    <p className="max-w-xs text-xs">How quickly traffic decreases. Higher = faster decay.</p>
                  </TooltipContent>
                </Tooltip>
              </div>
              <Badge variant="secondary" className="text-xs font-mono">
                {params.decay_rate ?? 0.1}
              </Badge>
            </div>
            <Slider
              value={[params.decay_rate ?? 0.1]}
              onValueChange={([v]) => handleDistributionParamChange('decay_rate', v)}
              min={0.01}
              max={1}
              step={0.01}
            />
          </div>
        );

      default:
        return null;
    }
  };

  return (
    <div className="h-full flex flex-col bg-sidebar">
      <div className="px-4 py-3 border-b border-sidebar-border flex items-center gap-2">
        <Activity className="w-4 h-4 text-cache" />
        <h3 className="text-sm font-semibold text-foreground">Workload Configuration</h3>
      </div>

      <ScrollArea className="flex-1">
        <div className="p-4 space-y-6">
          {/* Section 1: Request Rate (RPS) - Prominent */}
          <div className="space-y-3">
            <div className="flex items-center gap-2">
              <Label htmlFor="base_rps" className="text-xs text-muted-foreground uppercase tracking-wider">
                Request Rate
              </Label>
              <Tooltip>
                <TooltipTrigger>
                  <HelpCircle className="w-3 h-3 text-muted-foreground" />
                </TooltipTrigger>
                <TooltipContent>
                  <p className="max-w-xs text-xs">Target request rate for this service</p>
                </TooltipContent>
              </Tooltip>
            </div>
            <div className="relative">
              <Input
                id="base_rps"
                type="number"
                min={1}
                value={workload.base_rps}
                onChange={(e) => handleChange('base_rps', parseInt(e.target.value) || 1)}
                className={cn(
                  "h-14 text-2xl font-mono pr-14",
                  "bg-input border-border",
                  "focus:border-primary focus:ring-2 focus:ring-primary/20",
                  "transition-all duration-200"
                )}
              />
              <span className="absolute right-4 top-1/2 -translate-y-1/2 text-sm text-muted-foreground font-medium">
                RPS
              </span>
            </div>
            <p className="text-[11px] text-muted-foreground">Target request rate for this service</p>
          </div>

          <Separator />

          {/* Section 2: Distribution Type */}
          <div className="space-y-3">
            <div className="flex items-center gap-2">
              <Label className="text-xs text-muted-foreground uppercase tracking-wider">
                Traffic Distribution Pattern
              </Label>
            </div>
            <Select
              value={workload.distribution}
              onValueChange={(v) => handleChange('distribution', v as DistributionType)}
            >
              <SelectTrigger className="bg-input border-border h-12">
                <SelectValue>
                  {DISTRIBUTION_OPTIONS.find(d => d.value === workload.distribution) && (
                    <div className="flex items-center gap-3">
                      <span className="text-primary">
                        {DISTRIBUTION_OPTIONS.find(d => d.value === workload.distribution)?.icon}
                      </span>
                      <div className="text-left">
                        <span className="font-medium">
                          {DISTRIBUTION_OPTIONS.find(d => d.value === workload.distribution)?.label}
                        </span>
                      </div>
                    </div>
                  )}
                </SelectValue>
              </SelectTrigger>
              <SelectContent className="bg-popover border-border">
                {DISTRIBUTION_OPTIONS.map((option) => (
                  <SelectItem 
                    key={option.value} 
                    value={option.value}
                    onMouseEnter={() => setHoveredDistribution(option.value)}
                    onMouseLeave={() => setHoveredDistribution(null)}
                    className="py-3"
                  >
                    <div className="flex items-center gap-3">
                      <span className={cn(
                        "transition-colors",
                        hoveredDistribution === option.value ? "text-primary" : "text-muted-foreground"
                      )}>
                        {option.icon}
                      </span>
                      <div className="flex flex-col">
                        <span className="font-medium">{option.label}</span>
                        <span className="text-[10px] text-muted-foreground">{option.description}</span>
                      </div>
                    </div>
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          {/* Section 3: Distribution Parameters - Dynamic */}
          <div className="space-y-3">
            <Label className="text-xs text-muted-foreground uppercase tracking-wider">
              Distribution Parameters
            </Label>
            <div className="p-4 bg-card/50 border border-border/50 rounded-xl">
              {renderDistributionParams()}
            </div>
          </div>

          <Separator />

          {/* Workload Type */}
          <div className="space-y-2">
            <div className="flex items-center gap-2">
              <Label className="text-xs text-muted-foreground uppercase tracking-wider">Pattern Type</Label>
              <Tooltip>
                <TooltipTrigger>
                  <HelpCircle className="w-3 h-3 text-muted-foreground" />
                </TooltipTrigger>
                <TooltipContent>
                  <p className="max-w-xs text-xs">
                    Steady: constant load. Bursty: random spikes. Ramp-up: gradual increase.
                  </p>
                </TooltipContent>
              </Tooltip>
            </div>
            <Select
              value={workload.type}
              onValueChange={(v) => handleChange('type', v as WorkloadType)}
            >
              <SelectTrigger className="bg-input border-border">
                <SelectValue />
              </SelectTrigger>
              <SelectContent className="bg-popover border-border">
                <SelectItem value="steady">Steady</SelectItem>
                <SelectItem value="bursty">Bursty</SelectItem>
                <SelectItem value="ramp-up">Ramp-up</SelectItem>
              </SelectContent>
            </Select>
          </div>

          {/* Duration */}
          <div className="space-y-2">
            <div className="flex items-center gap-2">
              <Label htmlFor="duration_ms" className="text-xs text-muted-foreground uppercase tracking-wider">
                Duration (ms)
              </Label>
              <Tooltip>
                <TooltipTrigger>
                  <HelpCircle className="w-3 h-3 text-muted-foreground" />
                </TooltipTrigger>
                <TooltipContent>
                  <p className="max-w-xs text-xs">Total simulation duration in milliseconds</p>
                </TooltipContent>
              </Tooltip>
            </div>
            <Input
              id="duration_ms"
              type="number"
              min={1000}
              step={1000}
              value={workload.duration_ms}
              onChange={(e) => handleChange('duration_ms', parseInt(e.target.value) || 1000)}
              className="bg-input border-border"
            />
            <p className="text-xs text-muted-foreground">
              = {(workload.duration_ms / 1000).toFixed(1)} seconds
            </p>
          </div>

          <Separator />

          {/* Spikes */}
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <Zap className="w-4 h-4 text-cache" />
                <Label className="text-xs text-muted-foreground uppercase tracking-wider">Spikes</Label>
              </div>
              <Button size="sm" variant="outline" onClick={handleAddSpike} className="h-7 px-2">
                <Plus className="w-4 h-4" />
              </Button>
            </div>

            {workload.spikes.length === 0 ? (
              <p className="text-xs text-muted-foreground text-center py-4">
                No spikes configured. Add spikes for traffic bursts.
              </p>
            ) : (
              <div className="space-y-3">
                {workload.spikes.map((spike, index) => (
                  <div key={spike.id} className="p-3 border border-border rounded-lg space-y-3">
                    <div className="flex items-center justify-between">
                      <span className="text-xs font-medium">Spike {index + 1}</span>
                      <Button
                        size="icon"
                        variant="ghost"
                        className="h-6 w-6"
                        onClick={() => handleDeleteSpike(spike.id)}
                      >
                        <Trash2 className="w-3 h-3 text-destructive" />
                      </Button>
                    </div>

                    <div className="grid grid-cols-3 gap-2">
                      <div className="space-y-1">
                        <Label className="text-[10px] text-muted-foreground">Time (ms)</Label>
                        <Input
                          type="number"
                          min={0}
                          value={spike.time_ms}
                          onChange={(e) =>
                            handleUpdateSpike(spike.id, { time_ms: parseInt(e.target.value) || 0 })
                          }
                          className="bg-input border-border h-7 text-xs"
                        />
                      </div>
                      <div className="space-y-1">
                        <Label className="text-[10px] text-muted-foreground">RPS</Label>
                        <Input
                          type="number"
                          min={1}
                          value={spike.rps}
                          onChange={(e) =>
                            handleUpdateSpike(spike.id, { rps: parseInt(e.target.value) || 1 })
                          }
                          className="bg-input border-border h-7 text-xs"
                        />
                      </div>
                      <div className="space-y-1">
                        <Label className="text-[10px] text-muted-foreground">Duration</Label>
                        <Input
                          type="number"
                          min={100}
                          value={spike.duration_ms}
                          onChange={(e) =>
                            handleUpdateSpike(spike.id, { duration_ms: parseInt(e.target.value) || 100 })
                          }
                          className="bg-input border-border h-7 text-xs"
                        />
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </ScrollArea>
    </div>
  );
};
