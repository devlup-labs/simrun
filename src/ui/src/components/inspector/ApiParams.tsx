import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { ApiParameters, DEFAULT_API_PARAMS } from '@/types/simulation';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { HelpCircle } from 'lucide-react';

interface ApiParamsProps {
  params: ApiParameters;
  onChange: (params: ApiParameters) => void;
}

export const ApiParams = ({ params, onChange }: ApiParamsProps) => {
  const values = { ...DEFAULT_API_PARAMS, ...params };

  const handleChange = (key: keyof ApiParameters, value: number) => {
    onChange({ ...values, [key]: value });
  };

  return (
    <div className="space-y-4">
      {/* Max Concurrency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="api_max_concurrency" className="text-xs text-muted-foreground uppercase tracking-wider">
            Max Concurrency
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Maximum concurrent requests the service can handle</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="api_max_concurrency"
          type="number"
          min={1}
          value={values.max_concurrency}
          onChange={(e) => handleChange('max_concurrency', parseInt(e.target.value) || 1)}
          className="bg-input border-border"
        />
      </div>

      {/* Processing Latency */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="processing_latency_ms" className="text-xs text-muted-foreground uppercase tracking-wider">
            Processing Latency (ms)
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Time to process a request in milliseconds</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="processing_latency_ms"
          type="number"
          min={0}
          value={values.processing_latency_ms}
          onChange={(e) => handleChange('processing_latency_ms', parseInt(e.target.value) || 0)}
          className="bg-input border-border"
        />
      </div>

      {/* Timeout */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="timeout_ms" className="text-xs text-muted-foreground uppercase tracking-wider">
            Timeout (ms)
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Maximum time to wait for a response before timing out</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="timeout_ms"
          type="number"
          min={0}
          value={values.timeout_ms}
          onChange={(e) => handleChange('timeout_ms', parseInt(e.target.value) || 0)}
          className="bg-input border-border"
        />
      </div>

      {/* Retry Count */}
      <div className="space-y-2">
        <div className="flex items-center gap-2">
          <Label htmlFor="retry_count" className="text-xs text-muted-foreground uppercase tracking-wider">
            Retry Count
          </Label>
          <Tooltip>
            <TooltipTrigger>
              <HelpCircle className="w-3 h-3 text-muted-foreground" />
            </TooltipTrigger>
            <TooltipContent>
              <p className="max-w-xs text-xs">Number of retry attempts on failure</p>
            </TooltipContent>
          </Tooltip>
        </div>
        <Input
          id="retry_count"
          type="number"
          min={0}
          max={10}
          value={values.retry_count}
          onChange={(e) => handleChange('retry_count', parseInt(e.target.value) || 0)}
          className="bg-input border-border"
        />
      </div>
    </div>
  );
};
