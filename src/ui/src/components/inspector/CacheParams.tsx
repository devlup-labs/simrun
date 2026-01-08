import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { CacheParameters, DEFAULT_CACHE_PARAMS } from '@/types/simulation';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { HelpCircle } from 'lucide-react';

interface CacheParamsProps {
  params: CacheParameters;
  onChange: (params: CacheParameters) => void;
}

export const CacheParams = ({ params, onChange }: CacheParamsProps) => {
  const values = { ...DEFAULT_CACHE_PARAMS, ...params };

  const handleChange = <K extends keyof CacheParameters>(key: K, value: CacheParameters[K]) => {
    onChange({ ...values, [key]: value });
  };

  return (
    <div className="space-y-4">
      {/* Max Concurrency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="cache_max_concurrency" className="text-xs text-muted-foreground uppercase tracking-wider">
            Max Concurrency
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Maximum concurrent connections to the cache</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="cache_max_concurrency"
          type="number"
          min={1}
          value={values.max_concurrency}
          onChange={(e) => handleChange('max_concurrency', parseInt(e.target.value) || 1)}
          className="bg-input border-border"
        />
      </div>

      {/* Hit Rate */}
      <div className="space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Label className="text-xs text-muted-foreground uppercase tracking-wider">
              Hit Rate
            </Label>
            <Tooltip>
              <TooltipTrigger>
                <HelpCircle className="w-3 h-3 text-muted-foreground" />
              </TooltipTrigger>
              <TooltipContent>
                <p className="max-w-xs text-xs">Expected cache hit rate (0-1)</p>
              </TooltipContent>
            </Tooltip>
          </div>
          <span className="text-xs font-mono text-muted-foreground">{(values.hit_rate * 100).toFixed(0)}%</span>
        </div>
        <Slider
          value={[values.hit_rate * 100]}
          onValueChange={([v]) => handleChange('hit_rate', v / 100)}
          min={0}
          max={100}
          step={1}
          className="py-2"
        />
      </div>

      {/* Eviction Policy */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label className="text-xs text-muted-foreground uppercase tracking-wider">
            Eviction Policy
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Strategy for removing items when cache is full</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Select
          value={values.eviction_policy}
          onValueChange={(v) => handleChange('eviction_policy', v as CacheParameters['eviction_policy'])}
        >
          <SelectTrigger className="bg-input border-border">
            <SelectValue />
          </SelectTrigger>
          <SelectContent>
            <SelectItem value="lru">LRU (Least Recently Used)</SelectItem>
            <SelectItem value="lfu">LFU (Least Frequently Used)</SelectItem>
            <SelectItem value="fifo">FIFO (First In First Out)</SelectItem>
            <SelectItem value="random">Random</SelectItem>
          </SelectContent>
        </Select>
      </div>
    </div>
  );
};
