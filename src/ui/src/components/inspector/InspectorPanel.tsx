import { useCallback, useState } from 'react';
import { X, Trash2, Copy, ChevronRight, ChevronDown, PanelRightClose, PanelRight } from 'lucide-react';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Button } from '@/components/ui/button';
import { Separator } from '@/components/ui/separator';
import { useArchitectureStore } from '@/store/architectureStore';
import { COMPONENT_DEFINITIONS, ComponentCategory, ComponentProfile } from '@/types/architecture';
import {
  DatabaseParameters,
  CacheParameters,
  ApiParameters,
  NetworkParameters,
  DEFAULT_DATABASE_PARAMS,
  DEFAULT_CACHE_PARAMS,
  DEFAULT_API_PARAMS,
  DEFAULT_NETWORK_PARAMS,
} from '@/types/simulation';
import { DatabaseParams } from './DatabaseParams';
import { CacheParams } from './CacheParams';
import { ApiParams } from './ApiParams';
import { NetworkParams } from './NetworkParams';
import { cn } from '@/lib/utils';
import { getComponentLogo } from '@/components/icons/ComponentLogos';
import { toast } from 'sonner';

const categoryGradients: Record<ComponentCategory, string> = {
  database: 'gradient-database',
  cache: 'gradient-cache',
  api: 'gradient-api',
  network: 'gradient-network',
};

interface CollapsibleSectionProps {
  title: string;
  defaultOpen?: boolean;
  children: React.ReactNode;
}

const CollapsibleSection = ({ title, defaultOpen = true, children }: CollapsibleSectionProps) => {
  const [isOpen, setIsOpen] = useState(defaultOpen);

  return (
    <div className="space-y-2">
      <button
        onClick={() => setIsOpen(!isOpen)}
        className="flex items-center gap-2 w-full text-xs text-muted-foreground uppercase tracking-wider font-semibold hover:text-foreground transition-colors"
      >
        {isOpen ? <ChevronDown className="w-3 h-3" /> : <ChevronRight className="w-3 h-3" />}
        {title}
      </button>
      {isOpen && <div className="animate-fade-in">{children}</div>}
    </div>
  );
};

export const InspectorPanel = () => {
  const [collapsed, setCollapsed] = useState(false);
  const { nodes, edges, selectedNodeId, selectedEdgeId, updateNode, deleteNode, selectNode, selectEdge } = useArchitectureStore();

  const selectedNode = nodes.find((n) => n.id === selectedNodeId);
  const selectedEdge = edges.find((e) => e.id === selectedEdgeId);

  const handleLabelChange = useCallback(
    (value: string) => {
      if (selectedNodeId) {
        updateNode(selectedNodeId, { label: value });
      }
    },
    [selectedNodeId, updateNode]
  );

  const handleParameterChange = useCallback(
    (params: DatabaseParameters | CacheParameters | ApiParameters | NetworkParameters) => {
      if (selectedNodeId) {
        updateNode(selectedNodeId, { parameters: params });
      }
    },
    [selectedNodeId, updateNode]
  );

  const handleDelete = useCallback(() => {
    if (selectedNodeId) {
      deleteNode(selectedNodeId);
      toast.success('Node deleted');
    }
  }, [selectedNodeId, deleteNode]);

  const handleClose = useCallback(() => {
    selectNode(null);
    selectEdge(null);
  }, [selectNode, selectEdge]);

  const handleCopyId = (id: string) => {
    navigator.clipboard.writeText(id);
    toast.success('ID copied to clipboard');
  };

  // Collapsed state
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

  // No selection state
  if (!selectedNode && !selectedEdge) {
    return (
      <div className="w-80 h-full glass-strong border-l border-border/50 flex flex-col animate-slide-in-right">
        <div className="px-4 py-4 border-b border-border/50 flex items-center justify-between">
          <h2 className="text-sm font-semibold text-foreground">Properties</h2>
          <Button variant="ghost" size="icon" onClick={() => setCollapsed(true)} className="h-8 w-8">
            <PanelRightClose className="w-4 h-4" />
          </Button>
        </div>
        <div className="flex-1 flex items-center justify-center p-6">
          <div className="text-center">
            <div className="w-16 h-16 rounded-2xl bg-card/50 border border-border/50 flex items-center justify-center mx-auto mb-4">
              <ChevronRight className="w-8 h-8 text-muted-foreground/30" />
            </div>
            <p className="text-sm text-muted-foreground">
              Select a node or connection<br />to view properties
            </p>
          </div>
        </div>
      </div>
    );
  }

  // Edge selected
  if (selectedEdge) {
    const sourceNode = nodes.find((n) => n.id === selectedEdge.source);
    const targetNode = nodes.find((n) => n.id === selectedEdge.target);
    const edgeParams = (selectedEdge.data?.parameters as NetworkParameters) || DEFAULT_NETWORK_PARAMS;

    return (
      <div className="w-80 h-full glass-strong border-l border-border/50 flex flex-col animate-slide-in-right">
        <div className="px-4 py-4 border-b border-border/50 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="w-8 h-8 rounded-lg gradient-network flex items-center justify-center">
              <span className="text-xs font-bold text-white">⟷</span>
            </div>
            <div>
              <h2 className="text-sm font-semibold text-foreground">Connection</h2>
              <p className="text-[10px] text-muted-foreground">Network Link</p>
            </div>
          </div>
          <Button variant="ghost" size="icon" onClick={handleClose} className="h-8 w-8">
            <X className="w-4 h-4" />
          </Button>
        </div>
        
        <div className="flex-1 overflow-y-auto p-4 space-y-6">
          <CollapsibleSection title="Connection">
            <div className="space-y-3">
              <div className="p-3 bg-card/50 rounded-xl border border-border/50">
                <Label className="text-[10px] text-muted-foreground uppercase tracking-wider">Source</Label>
                <p className="text-sm font-medium font-mono mt-1">{sourceNode?.data.label as string || 'Unknown'}</p>
              </div>
              <div className="text-center text-muted-foreground">↓</div>
              <div className="p-3 bg-card/50 rounded-xl border border-border/50">
                <Label className="text-[10px] text-muted-foreground uppercase tracking-wider">Target</Label>
                <p className="text-sm font-medium font-mono mt-1">{targetNode?.data.label as string || 'Unknown'}</p>
              </div>
            </div>
          </CollapsibleSection>

          <Separator className="bg-border/50" />

          <CollapsibleSection title="Link Parameters">
            <NetworkParams
              params={edgeParams}
              onChange={(params) => {
                const { setEdges } = useArchitectureStore.getState();
                const updatedEdges = edges.map((e) =>
                  e.id === selectedEdgeId ? { ...e, data: { ...e.data, parameters: params } } : e
                );
                setEdges(updatedEdges);
              }}
            />
          </CollapsibleSection>
          
          <Separator className="bg-border/50" />

          <CollapsibleSection title="Metadata" defaultOpen={false}>
            <div className="space-y-2">
              <div className="flex items-center justify-between p-2 bg-card/30 rounded-lg">
                <span className="text-xs text-muted-foreground">Edge ID</span>
                <Button
                  variant="ghost"
                  size="sm"
                  className="h-6 px-2 text-xs font-mono"
                  onClick={() => handleCopyId(selectedEdge.id)}
                >
                  <Copy className="w-3 h-3 mr-1" />
                  Copy
                </Button>
              </div>
              <p className="text-[10px] font-mono text-muted-foreground break-all px-2">{selectedEdge.id}</p>
            </div>
          </CollapsibleSection>
        </div>
      </div>
    );
  }

  // Node selected
  const componentDef = COMPONENT_DEFINITIONS.find(
    (c) => c.profile === selectedNode?.data.profile
  );
  const category = selectedNode?.data.category as ComponentCategory;
  const nodeParams = selectedNode?.data.parameters as Record<string, unknown> | undefined;
  const LogoComponent = getComponentLogo(selectedNode?.data.profile as ComponentProfile);

  const getDefaultParams = () => {
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
        return {};
    }
  };

  const currentParams = { ...getDefaultParams(), ...nodeParams };

  return (
    <div className="w-80 h-full glass-strong border-l border-border/50 flex flex-col animate-slide-in-right">
      {/* Header */}
      <div className="px-4 py-4 border-b border-border/50 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className={cn('w-10 h-10 rounded-xl flex items-center justify-center', categoryGradients[category])}>
            <LogoComponent size={24} className="drop-shadow-md" />
          </div>
          <div>
            <h2 className="text-sm font-semibold text-foreground">{selectedNode?.data.label as string}</h2>
            <p className="text-[10px] text-muted-foreground font-mono uppercase">{selectedNode?.data.profile as string}</p>
          </div>
        </div>
        <Button variant="ghost" size="icon" onClick={handleClose} className="h-8 w-8">
          <X className="w-4 h-4" />
        </Button>
      </div>

      {/* Content */}
      <div className="flex-1 overflow-y-auto p-4 space-y-6">
        {/* Display Name */}
        <CollapsibleSection title="Display Name">
          <Input
            value={selectedNode?.data.label as string || ''}
            onChange={(e) => handleLabelChange(e.target.value)}
            className="bg-card/50 border-border/50 focus:border-primary"
          />
        </CollapsibleSection>

        <Separator className="bg-border/50" />

        {/* Component Info */}
        <CollapsibleSection title="Component Info">
          <div className="space-y-3">
            <div className="flex items-center justify-between p-2 bg-card/30 rounded-lg">
              <span className="text-xs text-muted-foreground">Type</span>
              <span className="text-xs font-medium capitalize">{category}</span>
            </div>
            <div className="flex items-center justify-between p-2 bg-card/30 rounded-lg">
              <span className="text-xs text-muted-foreground">Profile</span>
              <span className="text-xs font-mono">{selectedNode?.data.profile as string}</span>
            </div>
            {componentDef && (
              <p className="text-xs text-muted-foreground px-2">{componentDef.description}</p>
            )}
          </div>
        </CollapsibleSection>

        <Separator className="bg-border/50" />

        {/* Category-specific Parameters */}
        <CollapsibleSection title="Parameters">
          {category === 'database' && (
            <DatabaseParams
              params={currentParams as DatabaseParameters}
              onChange={(params) => handleParameterChange(params)}
            />
          )}

          {category === 'cache' && (
            <CacheParams
              params={currentParams as CacheParameters}
              onChange={(params) => handleParameterChange(params)}
            />
          )}

          {category === 'api' && (
            <ApiParams
              params={currentParams as ApiParameters}
              onChange={(params) => handleParameterChange(params)}
            />
          )}

          {category === 'network' && (
            <NetworkParams
              params={currentParams as NetworkParameters}
              onChange={(params) => handleParameterChange(params)}
            />
          )}
        </CollapsibleSection>

        <Separator className="bg-border/50" />

        {/* Metadata */}
        <CollapsibleSection title="Metadata" defaultOpen={false}>
          <div className="space-y-3">
            <div className="flex items-center justify-between p-2 bg-card/30 rounded-lg">
              <span className="text-xs text-muted-foreground">Node ID</span>
              <Button
                variant="ghost"
                size="sm"
                className="h-6 px-2 text-xs font-mono"
                onClick={() => handleCopyId(selectedNode?.id || '')}
              >
                <Copy className="w-3 h-3 mr-1" />
                Copy
              </Button>
            </div>
            <p className="text-[10px] font-mono text-muted-foreground break-all px-2">{selectedNode?.id}</p>
            <div className="flex items-center justify-between p-2 bg-card/30 rounded-lg">
              <span className="text-xs text-muted-foreground">Position</span>
              <span className="text-xs font-mono">
                {Math.round(selectedNode?.position.x || 0)}, {Math.round(selectedNode?.position.y || 0)}
              </span>
            </div>
          </div>
        </CollapsibleSection>
      </div>

      {/* Actions */}
      <div className="p-4 border-t border-border/50">
        <Button
          variant="destructive"
          className="w-full gap-2"
          onClick={handleDelete}
        >
          <Trash2 className="w-4 h-4" />
          Delete Node
        </Button>
      </div>
    </div>
  );
};
