import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { NetworkParameters, DEFAULT_NETWORK_PARAMS } from '@/types/simulation';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { HelpCircle } from 'lucide-react';

interface NetworkParamsProps {
  params: NetworkParameters;
  onChange: (params: NetworkParameters) => void;
}

export const NetworkParams = ({ params, onChange }: NetworkParamsProps) => {
  const values = { ...DEFAULT_NETWORK_PARAMS, ...params };

  const handleChange = (key: keyof NetworkParameters, value: number | undefined) => {
    onChange({ ...values, [key]: value });
  };

  return (
    <div className="space-y-4">
      {/* Latency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="latency_ms" className="text-xs text-muted-foreground uppercase tracking-wider">
            Latency (ms)
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Network latency in milliseconds</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="latency_ms"
          type="number"
          min={0}
          value={values.latency_ms}
          onChange={(e) => handleChange('latency_ms', parseInt(e.target.value) || 0)}
          className="bg-input border-border"
        />
      </div>

      {/* Loss Probability */}
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Label className="text-xs text-muted-foreground uppercase tracking-wider">
              Loss Probability
            </Label>
            <Tooltip>
              <TooltipTrigger>
                <HelpCircle className="w-3 h-3 text-muted-foreground" />
              </TooltipTrigger>
              <TooltipContent>
                <p className="max-w-xs text-xs">Probability of packet loss (0-1)</p>
              </TooltipContent>
            </Tooltip>
          </div>
          <span className="text-xs font-mono text-muted-foreground">{(values.loss_prob * 100).toFixed(1)}%</span>
        </div>
        <Slider
          value={[values.loss_prob * 100]}
          onValueChange={([v]) => handleChange('loss_prob', v / 100)}
          min={0}
          max={50}
          step={0.5}
          className="py-2"
        />
      </div>

      {/* Bandwidth Limit */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="bandwidth_limit" className="text-xs text-muted-foreground uppercase tracking-wider">
            Bandwidth Limit (Mbps)
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Maximum bandwidth in Mbps (leave empty for unlimited)</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="bandwidth_limit"
          type="number"
          min={0}
          placeholder="Unlimited"
          value={values.bandwidth_limit ?? ''}
          onChange={(e) => handleChange('bandwidth_limit', e.target.value ? parseInt(e.target.value) : undefined)}
          className="bg-input border-border"
        />
      </div>
    </div>
  );
};
