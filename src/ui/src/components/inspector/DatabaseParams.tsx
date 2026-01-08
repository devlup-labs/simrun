import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { DatabaseParameters, DEFAULT_DATABASE_PARAMS } from '@/types/simulation';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { HelpCircle } from 'lucide-react';

interface DatabaseParamsProps {
  params: DatabaseParameters;
  onChange: (params: DatabaseParameters) => void;
}

export const DatabaseParams = ({ params, onChange }: DatabaseParamsProps) => {
  const values = { ...DEFAULT_DATABASE_PARAMS, ...params };

  const handleChange = (key: keyof DatabaseParameters, value: number) => {
    onChange({ ...values, [key]: value });
  };

  return (
    <div className="space-y-4">
      {/* Max Concurrency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="max_concurrency" className="text-xs text-muted-foreground uppercase tracking-wider">
            Max Concurrency
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Maximum number of concurrent connections the database can handle</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="max_concurrency"
          type="number"
          min={1}
          value={values.max_concurrency}
          onChange={(e) => handleChange('max_concurrency', parseInt(e.target.value) || 1)}
          className="bg-input border-border"
        />
      </div>

      {/* Base Latency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="base_latency_ms" className="text-xs text-muted-foreground uppercase tracking-wider">
            Base Latency (ms)
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Average query response time in milliseconds</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="base_latency_ms"
          type="number"
          min={0}
          value={values.base_latency_ms}
          onChange={(e) => handleChange('base_latency_ms', parseInt(e.target.value) || 0)}
          className="bg-input border-border"
        />
      </div>

      {/* Disk Fail Probability */}
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Label className="text-xs text-muted-foreground uppercase tracking-wider">
              Disk Fail Prob
            </Label>
            <Tooltip>
              <TooltipTrigger>
                <HelpCircle className="w-3 h-3 text-muted-foreground" />
              </TooltipTrigger>
              <TooltipContent>
                <p className="max-w-xs text-xs">Probability of disk I/O failure (0-1)</p>
              </TooltipContent>
            </Tooltip>
          </div>
          <span className="text-xs font-mono text-muted-foreground">{values.disk_fail_prob.toFixed(3)}</span>
        </div>
        <Slider
          value={[values.disk_fail_prob * 100]}
          onValueChange={([v]) => handleChange('disk_fail_prob', v / 100)}
          min={0}
          max={10}
          step={0.1}
          className="py-2"
        />
      </div>

      {/* Read/Write Ratio */}
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Label className="text-xs text-muted-foreground uppercase tracking-wider">
              Read/Write Ratio
            </Label>
            <Tooltip>
              <TooltipTrigger>
                <HelpCircle className="w-3 h-3 text-muted-foreground" />
              </TooltipTrigger>
              <TooltipContent>
                <p className="max-w-xs text-xs">Ratio of read operations to total operations (0-1)</p>
              </TooltipContent>
            </Tooltip>
          </div>
          <span className="text-xs font-mono text-muted-foreground">{((values.read_write_ratio || 0.5) * 100).toFixed(0)}% reads</span>
        </div>
        <Slider
          value={[(values.read_write_ratio || 0.5) * 100]}
          onValueChange={([v]) => handleChange('read_write_ratio', v / 100)}
          min={0}
          max={100}
          step={1}
          className="py-2"
        />
      </div>
    </div>
  );
};
