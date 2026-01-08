import { useState } from 'react';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { RoutesPanel } from './RoutesPanel';
import { WorkloadPanel } from './WorkloadPanel';
import { FaultsPanel } from './FaultsPanel';
import { Route, Activity, AlertTriangle, PanelRightClose, PanelRight } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { cn } from '@/lib/utils';

export const ConfigTabs = () => {
  const [collapsed, setCollapsed] = useState(false);

  if (collapsed) {
    return (
      <div className="w-12 h-full glass-strong border-l border-border/50 flex flex-col items-center py-4">
        <Button
          variant="ghost"
          size="icon"
          onClick={() => setCollapsed(false)}
          className="h-8 w-8"
        >
          <PanelRight className="w-4 h-4" />
        </Button>
      </div>
    );
  }

  return (
    <div className="w-80 h-full glass-strong border-l border-border/50 flex flex-col">
      <Tabs defaultValue="routes" className="h-full flex flex-col">
        <div className="px-3 pt-3 pb-2 border-b border-border/50 flex items-center justify-between">
          <TabsList className="grid grid-cols-3 h-9 bg-card/50 p-1">
            <TabsTrigger 
              value="routes" 
              className={cn(
                "text-xs gap-1.5 px-3 data-[state=active]:bg-primary data-[state=active]:text-primary-foreground",
                "transition-all duration-200"
              )}
            >
              <Route className="w-3.5 h-3.5" />
              Routes
            </TabsTrigger>
            <TabsTrigger 
              value="workload" 
              className={cn(
                "text-xs gap-1.5 px-3 data-[state=active]:bg-primary data-[state=active]:text-primary-foreground",
                "transition-all duration-200"
              )}
            >
              <Activity className="w-3.5 h-3.5" />
              Workload
            </TabsTrigger>
            <TabsTrigger 
              value="faults" 
              className={cn(
                "text-xs gap-1.5 px-3 data-[state=active]:bg-primary data-[state=active]:text-primary-foreground",
                "transition-all duration-200"
              )}
            >
              <AlertTriangle className="w-3.5 h-3.5" />
              Faults
            </TabsTrigger>
          </TabsList>
          <Button
            variant="ghost"
            size="icon"
            onClick={() => setCollapsed(true)}
            className="h-8 w-8 shrink-0"
          >
            <PanelRightClose className="w-4 h-4" />
          </Button>
        </div>

        <TabsContent value="routes" className="flex-1 m-0 overflow-hidden">
          <RoutesPanel />
        </TabsContent>

        <TabsContent value="workload" className="flex-1 m-0 overflow-hidden">
          <WorkloadPanel />
        </TabsContent>

        <TabsContent value="faults" className="flex-1 m-0 overflow-hidden">
          <FaultsPanel />
        </TabsContent>
      </Tabs>
    </div>
  );
};
