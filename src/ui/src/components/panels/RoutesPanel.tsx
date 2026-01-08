import { useState } from 'react';
import { Plus, Trash2, GripVertical, Route as RouteIcon } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Separator } from '@/components/ui/separator';
import { ScrollArea } from '@/components/ui/scroll-area';
import { useSimulationStore } from '@/store/simulationStore';
import { useArchitectureStore } from '@/store/architectureStore';
import { Route } from '@/types/simulation';
import { cn } from '@/lib/utils';

export const RoutesPanel = () => {
  const { routes, addRoute, updateRoute, deleteRoute } = useSimulationStore();
  const { nodes } = useArchitectureStore();
  const [expandedRoute, setExpandedRoute] = useState<string | null>(null);

  // Filter to only API nodes for entry points
  const apiNodes = nodes.filter((n) => n.data.category === 'api');
  const allNodes = nodes.filter((n) => n.data.category !== 'network');

  const handleAddRoute = () => {
    const newRoute: Route = {
      id: `route_${Date.now()}`,
      name: `Route ${routes.length + 1}`,
      entryNodeId: apiNodes[0]?.id || '',
      path: [],
      weight: 100 / (routes.length + 1),
    };
    addRoute(newRoute);
    setExpandedRoute(newRoute.id);
  };

  const handleAddNodeToPath = (routeId: string, nodeId: string) => {
    const route = routes.find((r) => r.id === routeId);
    if (route && !route.path.includes(nodeId)) {
      updateRoute(routeId, { path: [...route.path, nodeId] });
    }
  };

  const handleRemoveNodeFromPath = (routeId: string, index: number) => {
    const route = routes.find((r) => r.id === routeId);
    if (route) {
      const newPath = [...route.path];
      newPath.splice(index, 1);
      updateRoute(routeId, { path: newPath });
    }
  };

  const getNodeLabel = (nodeId: string) => {
    const node = nodes.find((n) => n.id === nodeId);
    return (node?.data.label as string) || nodeId;
  };

  // Calculate total weight
  const totalWeight = routes.reduce((sum, r) => sum + r.weight, 0);

  return (
    <div className="h-full flex flex-col bg-sidebar">
      <div className="px-4 py-3 border-b border-sidebar-border flex items-center justify-between">
        <div className="flex items-center gap-2">
          <RouteIcon className="w-4 h-4 text-primary" />
          <h3 className="text-sm font-semibold text-foreground">Routes</h3>
        </div>
        <Button size="sm" variant="outline" onClick={handleAddRoute} className="h-7 px-2">
          <Plus className="w-4 h-4" />
        </Button>
      </div>

      <ScrollArea className="flex-1">
        <div className="p-4 space-y-4">
          {routes.length === 0 ? (
            <p className="text-sm text-muted-foreground text-center py-8">
              No routes defined. Add a route to define request paths.
            </p>
          ) : (
            routes.map((route) => (
              <div
                key={route.id}
                className={cn(
                  'border border-border rounded-lg overflow-hidden',
                  expandedRoute === route.id && 'border-primary/50'
                )}
              >
                {/* Route Header */}
                <div
                  className="px-3 py-2 bg-muted/50 flex items-center justify-between cursor-pointer"
                  onClick={() => setExpandedRoute(expandedRoute === route.id ? null : route.id)}
                >
                  <div className="flex items-center gap-2">
                    <GripVertical className="w-4 h-4 text-muted-foreground" />
                    <span className="text-sm font-medium">{route.name}</span>
                  </div>
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-mono text-muted-foreground">{route.weight.toFixed(0)}%</span>
                    <Button
                      size="icon"
                      variant="ghost"
                      className="h-6 w-6"
                      onClick={(e) => {
                        e.stopPropagation();
                        deleteRoute(route.id);
                      }}
                    >
                      <Trash2 className="w-3 h-3 text-destructive" />
                    </Button>
                  </div>
                </div>

                {/* Route Details */}
                {expandedRoute === route.id && (
                  <div className="p-3 space-y-4 animate-fade-in">
                    {/* Route Name */}
                    <div className="space-y-2">
                      <Label className="text-xs text-muted-foreground uppercase tracking-wider">Name</Label>
                      <Input
                        value={route.name}
                        onChange={(e) => updateRoute(route.id, { name: e.target.value })}
                        className="bg-input border-border h-8"
                      />
                    </div>

                    {/* Entry Node */}
                    <div className="space-y-2">
                      <Label className="text-xs text-muted-foreground uppercase tracking-wider">Entry Node (API)</Label>
                      <Select
                        value={route.entryNodeId}
                        onValueChange={(v) => updateRoute(route.id, { entryNodeId: v })}
                      >
                        <SelectTrigger className="bg-input border-border h-8">
                          <SelectValue placeholder="Select API entry point" />
                        </SelectTrigger>
                        <SelectContent>
                          {apiNodes.map((node) => (
                            <SelectItem key={node.id} value={node.id}>
                              {node.data.label as string}
                            </SelectItem>
                          ))}
                        </SelectContent>
                      </Select>
                    </div>

                    {/* Path */}
                    <div className="space-y-2">
                      <Label className="text-xs text-muted-foreground uppercase tracking-wider">Path</Label>
                      <div className="space-y-1">
                        {route.path.map((nodeId, index) => (
                          <div key={index} className="flex items-center gap-2">
                            <span className="text-xs font-mono text-muted-foreground w-4">{index + 1}</span>
                            <span className="flex-1 text-sm px-2 py-1 bg-muted rounded">{getNodeLabel(nodeId)}</span>
                            <Button
                              size="icon"
                              variant="ghost"
                              className="h-6 w-6"
                              onClick={() => handleRemoveNodeFromPath(route.id, index)}
                            >
                              <Trash2 className="w-3 h-3" />
                            </Button>
                          </div>
                        ))}
                      </div>
                      <Select onValueChange={(v) => handleAddNodeToPath(route.id, v)}>
                        <SelectTrigger className="bg-input border-border h-8">
                          <SelectValue placeholder="Add node to path..." />
                        </SelectTrigger>
                        <SelectContent>
                          {allNodes
                            .filter((n) => !route.path.includes(n.id))
                            .map((node) => (
                              <SelectItem key={node.id} value={node.id}>
                                {node.data.label as string}
                              </SelectItem>
                            ))}
                        </SelectContent>
                      </Select>
                    </div>

                    {/* Weight */}
                    <div className="space-y-2">
                      <Label className="text-xs text-muted-foreground uppercase tracking-wider">Traffic Weight (%)</Label>
                      <Input
                        type="number"
                        min={0}
                        max={100}
                        value={route.weight}
                        onChange={(e) => updateRoute(route.id, { weight: parseFloat(e.target.value) || 0 })}
                        className="bg-input border-border h-8"
                      />
                    </div>
                  </div>
                )}
              </div>
            ))
          )}

          {routes.length > 0 && (
            <>
              <Separator />
              <div className="flex items-center justify-between text-xs">
                <span className="text-muted-foreground">Total Weight</span>
                <span className={cn('font-mono', totalWeight !== 100 && 'text-destructive')}>
                  {totalWeight.toFixed(0)}%
                </span>
              </div>
            </>
          )}
        </div>
      </ScrollArea>
    </div>
  );
};
