import { useState } from 'react';
import { Download, Upload, Trash2, Play, Terminal, Copy, Check, Route, AlertTriangle, Boxes, Link2, Moon, Sun } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Separator } from '@/components/ui/separator';
import { Badge } from '@/components/ui/badge';
import { useArchitectureStore } from '@/store/architectureStore';
import { useSimulationStore } from '@/store/simulationStore';
import { useTheme } from '@/hooks/use-theme';
import {
  SimulationExport,
  DEFAULT_DATABASE_PARAMS,
  DEFAULT_CACHE_PARAMS,
  DEFAULT_API_PARAMS,
  DEFAULT_NETWORK_PARAMS,
  ComponentParameters,
  NetworkParameters,
} from '@/types/simulation';
import { ComponentCategory } from '@/types/architecture';
import { toast } from 'sonner';
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from '@/components/ui/alert-dialog';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { cn } from '@/lib/utils';

type CanvasHeaderProps = {
  onRunSimulation?: () => void;
};

export const CanvasHeader = ({ onRunSimulation }: CanvasHeaderProps) => {
  const { nodes, edges, importGraph, clearCanvas } = useArchitectureStore();
  const { routes, workload, faults, clearAll: clearSimulation } = useSimulationStore();
  const { theme, toggleTheme } = useTheme();
  const [showRunDialog, setShowRunDialog] = useState(false);
  const [exportedFileName, setExportedFileName] = useState('');
  const [copiedCommand, setCopiedCommand] = useState(false);
  const [projectName, setProjectName] = useState('Untitled Project');

  const getDefaultParams = (category: ComponentCategory): ComponentParameters => {
    switch (category) {
      case 'database':
        return DEFAULT_DATABASE_PARAMS;
      case 'cache':
        return DEFAULT_CACHE_PARAMS;
      case 'api':
        return DEFAULT_API_PARAMS;
      case 'network':
        return DEFAULT_NETWORK_PARAMS;
      default:
        return DEFAULT_API_PARAMS;
    }
  };

  const buildExport = (): SimulationExport => {
    return {
      components: nodes.map((node) => ({
        id: node.id,
        type: node.data.category as string,
        profile: node.data.profile as string,
        position: node.position,
        label: node.data.label as string,
        parameters: {
          ...getDefaultParams(node.data.category as ComponentCategory),
          ...(node.data.parameters as ComponentParameters),
        },
      })),
      links: edges.map((edge) => ({
        id: edge.id,
        source: edge.source,
        target: edge.target,
        parameters: {
          ...DEFAULT_NETWORK_PARAMS,
          ...((edge.data?.parameters as NetworkParameters) || {}),
        },
      })),
      routes,
      workload,
      faults,
      metadata: {
        version: '1.0.0',
        name: projectName,
        createdAt: new Date().toISOString(),
      },
    };
  };

  const handleExport = () => {
    const exportData = buildExport();
    const dataStr = JSON.stringify(exportData, null, 2);
    const dataUri = 'data:application/json;charset=utf-8,' + encodeURIComponent(dataStr);
    
    const exportFileDefaultName = `simrun-${projectName.toLowerCase().replace(/\s+/g, '-')}-${Date.now()}.json`;
    
    const linkElement = document.createElement('a');
    linkElement.setAttribute('href', dataUri);
    linkElement.setAttribute('download', exportFileDefaultName);
    linkElement.click();
    
    toast.success('Architecture exported successfully');
    return exportFileDefaultName;
  };

  const handleRunSimulation = () => {
    const fileName = handleExport();
    setExportedFileName(fileName);
    setShowRunDialog(true);
  };

  const handleCopyCommand = () => {
    navigator.clipboard.writeText(`simrun.exe --input "${exportedFileName}"`);
    setCopiedCommand(true);
    toast.success('Command copied to clipboard');
    setTimeout(() => setCopiedCommand(false), 2000);
  };

  const handleImport = () => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    
    input.onchange = (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (!file) return;
      
      const reader = new FileReader();
      reader.onload = (event) => {
        try {
          const data = JSON.parse(event.target?.result as string);
          
          if (data.components) {
            const graphData = {
              nodes: data.components.map((c: any) => ({
                id: c.id,
                type: c.type,
                profile: c.profile,
                position: c.position,
                data: {
                  label: c.label,
                  profile: c.profile,
                  category: c.type,
                  parameters: c.parameters,
                },
              })),
              edges: data.links.map((l: any) => ({
                id: l.id,
                source: l.source,
                target: l.target,
                data: { parameters: l.parameters },
              })),
            };
            importGraph(graphData);
            
            if (data.metadata?.name) {
              setProjectName(data.metadata.name);
            }
            
            if (data.routes || data.workload || data.faults) {
              const simStore = useSimulationStore.getState();
              if (data.routes) {
                simStore.clearAll();
                data.routes.forEach((r: any) => simStore.addRoute(r));
              }
              if (data.workload) simStore.setWorkload(data.workload);
              if (data.faults) {
                data.faults.forEach((f: any) => simStore.addFault(f));
              }
            }
          } else {
            importGraph(data);
          }
          
          toast.success('Architecture imported successfully');
        } catch {
          toast.error('Failed to import architecture. Invalid JSON file.');
        }
      };
      reader.readAsText(file);
    };
    
    input.click();
  };

  const handleClear = () => {
    clearCanvas();
    clearSimulation();
    setProjectName('Untitled Project');
    toast.success('Canvas cleared');
  };

  return (
    <>
      <header className="h-14 glass-strong border-b border-border/50 flex items-center justify-between px-4">
        {/* Left Section - Logo and Project Name */}
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg gradient-primary flex items-center justify-center shadow-lg">
              <span className="text-primary-foreground font-bold text-sm">Σ</span>
            </div>
            <Input 
              value={projectName}
              onChange={(e) => setProjectName(e.target.value)}
              className="w-48 h-8 bg-transparent border-transparent hover:border-border focus:border-primary 
                         text-sm font-medium transition-colors"
              placeholder="Project name..."
            />
          </div>
        </div>

        {/* Center Section - Stats */}
        <div className="flex items-center gap-3">
          <Badge variant="secondary" className="gap-1.5 px-3 py-1 bg-card/50">
            <Boxes className="w-3.5 h-3.5 text-primary" />
            <span className="font-mono text-xs">{nodes.length}</span>
          </Badge>
          <Badge variant="secondary" className="gap-1.5 px-3 py-1 bg-card/50">
            <Link2 className="w-3.5 h-3.5 text-muted-foreground" />
            <span className="font-mono text-xs">{edges.length}</span>
          </Badge>
          {routes.length > 0 && (
            <Badge variant="secondary" className="gap-1.5 px-3 py-1 bg-card/50">
              <Route className="w-3.5 h-3.5 text-api" />
              <span className="font-mono text-xs">{routes.length}</span>
            </Badge>
          )}
          {faults.length > 0 && (
            <Badge variant="secondary" className="gap-1.5 px-3 py-1 bg-card/50">
              <AlertTriangle className="w-3.5 h-3.5 text-destructive" />
              <span className="font-mono text-xs">{faults.length}</span>
            </Badge>
          )}
        </div>

        {/* Right Section - Actions */}
        <div className="flex items-center gap-2">
          {/* Theme Toggle */}
          <Button
            variant="ghost"
            size="icon"
            onClick={toggleTheme}
            className="h-8 w-8 transition-all duration-300"
          >
            {theme === 'dark' ? (
              <Sun className="w-4 h-4 text-cache transition-transform duration-300 rotate-0 scale-100" />
            ) : (
              <Moon className="w-4 h-4 text-primary transition-transform duration-300 rotate-0 scale-100" />
            )}
          </Button>

          <Separator orientation="vertical" className="h-6 mx-1" />

          <Button
            variant="ghost"
            size="sm"
            onClick={handleImport}
            className="gap-2 h-8"
          >
            <Upload className="w-4 h-4" />
            <span className="hidden sm:inline">Import</span>
          </Button>
          
          <Button
            variant="ghost"
            size="sm"
            onClick={handleExport}
            className="gap-2 h-8"
            disabled={nodes.length === 0}
          >
            <Download className="w-4 h-4" />
            <span className="hidden sm:inline">Export</span>
          </Button>
          
          <Separator orientation="vertical" className="h-6 mx-1" />

          {/* Run Simulation Button */}
          <Button
            size="sm"
            onClick={() => onRunSimulation?.()}
            className={cn(
              'gap-2 h-8 btn-premium gradient-primary border-0',
              'hover:opacity-90 transition-opacity'
            )}
            disabled={nodes.length === 0}
          >
            <Play className="w-4 h-4" />
            <span>Run Simulation</span>
          </Button>
          
          <Separator orientation="vertical" className="h-6 mx-1" />
          
          <AlertDialog>
            <AlertDialogTrigger asChild>
              <Button
                variant="ghost"
                size="sm"
                className="gap-2 h-8 text-destructive hover:text-destructive hover:bg-destructive/10"
                disabled={nodes.length === 0}
              >
                <Trash2 className="w-4 h-4" />
                <span className="hidden sm:inline">Clear</span>
              </Button>
            </AlertDialogTrigger>
            <AlertDialogContent className="glass-strong">
              <AlertDialogHeader>
                <AlertDialogTitle>Clear Canvas</AlertDialogTitle>
                <AlertDialogDescription>
                  This will remove all nodes, connections, routes, workload settings, and faults. This action cannot be undone.
                </AlertDialogDescription>
              </AlertDialogHeader>
              <AlertDialogFooter>
                <AlertDialogCancel>Cancel</AlertDialogCancel>
                <AlertDialogAction onClick={handleClear} className="bg-destructive hover:bg-destructive/90">
                  Clear All
                </AlertDialogAction>
              </AlertDialogFooter>
            </AlertDialogContent>
          </AlertDialog>
        </div>
      </header>

      {/* Run Simulation Dialog */}
      <Dialog open={showRunDialog} onOpenChange={setShowRunDialog}>
        <DialogContent className="sm:max-w-lg glass-strong">
          <DialogHeader>
            <DialogTitle className="flex items-center gap-2">
              <div className="w-8 h-8 rounded-lg gradient-primary flex items-center justify-center">
                <Terminal className="w-4 h-4 text-primary-foreground" />
              </div>
              Run Simulation
            </DialogTitle>
            <DialogDescription>
              Your architecture has been exported with all configuration. Run the simulation using the SimRUN executable.
            </DialogDescription>
          </DialogHeader>
          
          <div className="space-y-4 mt-4">
            {/* Summary */}
            <div className="p-3 bg-card/50 border border-border/50 rounded-xl">
              <div className="flex items-center gap-4 text-xs">
                <div className="flex items-center gap-1.5">
                  <Boxes className="w-3.5 h-3.5 text-primary" />
                  <span className="font-mono">{nodes.length} components</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <Link2 className="w-3.5 h-3.5 text-muted-foreground" />
                  <span className="font-mono">{edges.length} links</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <Route className="w-3.5 h-3.5 text-api" />
                  <span className="font-mono">{routes.length} routes</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <AlertTriangle className="w-3.5 h-3.5 text-destructive" />
                  <span className="font-mono">{faults.length} faults</span>
                </div>
              </div>
            </div>

            {/* Steps */}
            <div className="space-y-4">
              {/* Step 1 */}
              <div className="flex gap-3">
                <div className="flex-shrink-0 w-7 h-7 rounded-full gradient-primary flex items-center justify-center text-sm font-bold text-primary-foreground">
                  1
                </div>
                <div className="flex-1 pt-0.5">
                  <p className="text-sm font-medium text-foreground">Architecture Exported</p>
                  <p className="text-xs text-muted-foreground mt-1">
                    File saved as <code className="px-1.5 py-0.5 bg-card rounded text-xs font-mono">{exportedFileName}</code>
                  </p>
                </div>
              </div>

              {/* Step 2 */}
              <div className="flex gap-3">
                <div className="flex-shrink-0 w-7 h-7 rounded-full gradient-primary flex items-center justify-center text-sm font-bold text-primary-foreground">
                  2
                </div>
                <div className="flex-1 pt-0.5">
                  <p className="text-sm font-medium text-foreground">Open Terminal</p>
                  <p className="text-xs text-muted-foreground mt-1">
                    Navigate to your SimRUN installation directory
                  </p>
                </div>
              </div>

              {/* Step 3 */}
              <div className="flex gap-3">
                <div className="flex-shrink-0 w-7 h-7 rounded-full gradient-primary flex items-center justify-center text-sm font-bold text-primary-foreground">
                  3
                </div>
                <div className="flex-1 pt-0.5">
                  <p className="text-sm font-medium text-foreground">Run Command</p>
                  <div className="mt-2 flex items-center gap-2">
                    <code className="flex-1 px-3 py-2.5 bg-card border border-border/50 rounded-lg text-xs font-mono text-foreground overflow-x-auto">
                      simrun.exe --input "{exportedFileName}"
                    </code>
                    <Button
                      variant="outline"
                      size="icon"
                      className="shrink-0 h-9 w-9"
                      onClick={handleCopyCommand}
                    >
                      {copiedCommand ? (
                        <Check className="w-4 h-4 text-api" />
                      ) : (
                        <Copy className="w-4 h-4" />
                      )}
                    </Button>
                  </div>
                </div>
              </div>
            </div>

            {/* Info box */}
            <div className="mt-4 p-3 bg-primary/5 border border-primary/20 rounded-xl">
              <p className="text-xs text-muted-foreground">
                <strong className="text-foreground">Note:</strong> Make sure the exported JSON file is in the same directory as simrun.exe, or provide the full path to the file.
              </p>
            </div>
          </div>

          <div className="flex justify-end gap-2 mt-4">
            <Button variant="outline" onClick={() => setShowRunDialog(false)}>
              Close
            </Button>
          </div>
        </DialogContent>
      </Dialog>
    </>
  );
};
