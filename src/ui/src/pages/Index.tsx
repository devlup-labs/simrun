import { ComponentToolbar } from '@/components/toolbar/ComponentToolbar';
import { ArchitectureCanvas } from '@/components/canvas/ArchitectureCanvas';
import { InspectorPanel } from '@/components/inspector/InspectorPanel';
import { CanvasHeader } from '@/components/header/CanvasHeader';
import { ConfigTabs } from '@/components/panels/ConfigTabs';
import { TooltipProvider } from '@/components/ui/tooltip';
import { useArchitectureStore } from '@/store/architectureStore';
import { useSimulationStore } from '@/store/simulationStore';
import { useState } from "react";
import { toast } from "sonner";
import { serializeToIR, wrapForCompiler } from '@/lib/irSerializer';
import { ComponentCategory } from '@/types/architecture';
import { ComponentParameters, NetworkParameters, DEFAULT_DATABASE_PARAMS, DEFAULT_CACHE_PARAMS, DEFAULT_API_PARAMS, DEFAULT_NETWORK_PARAMS } from '@/types/simulation';

const Index = () => {
  const [simulationResult, setSimulationResult] = useState<any>(null);
  const [isOutputMinimized, setIsOutputMinimized] = useState(false);
  const [isOutputClosed, setIsOutputClosed] = useState(false);
  const { nodes, edges } = useArchitectureStore();
  const { routes, workload, faults } = useSimulationStore();

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

  const handleRunSimulation = async () => {
    if (nodes.length === 0) {
      toast.error("Please add components to the canvas");
      return;
    }

    try {
      // Build the architecture export from canvas state
      const exportData = {
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
          createdAt: new Date().toISOString(),
        },
      };

      console.log("[UI] Export data:", exportData);
      console.log("[UI] Components count:", exportData.components.length);

      // Serialize to IR format
      const ir = serializeToIR(exportData);
      console.log("[UI] Serialized IR:", ir);
      console.log("[UI] IR Components:", ir.context.components);
      
      const compilerRequest = wrapForCompiler(ir);
      console.log("[UI] Final request to compiler:", compilerRequest);

      toast.promise(
        window.api.simulate(compilerRequest),
        {
          loading: 'Running simulation...',
          success: (result) => {
            console.log("[UI] Response from compiler:", result);
            setSimulationResult(result);
            setIsOutputClosed(false);
            setIsOutputMinimized(false);
            return 'Simulation completed';
          },
          error: (err) => {
            console.error("[UI] Error from compiler:", err);
            return 'Compiler error - check console';
          }
        }
      );
    } catch (err) {
      console.error("[UI] Failed to prepare simulation:", err);
      toast.error("Failed to prepare simulation");
    }
  };

  return (
    <TooltipProvider>
      <div className="flex h-screen w-full overflow-hidden bg-background">
        {/* Left Toolbar */}
        <ComponentToolbar />

        {/* Main Content Area */}
        <div className="flex-1 flex flex-col min-w-0">
          {/* Header */}
          <CanvasHeader onRunSimulation={handleRunSimulation} />

          {/* Canvas */}
          <ArchitectureCanvas />
        </div>

        {/* Right Side: Inspector + Config Tabs */}
        <div className="flex">
          <InspectorPanel />
          <ConfigTabs />
        </div>

        {/* Simulation Result Overlay */}
        {simulationResult && !isOutputClosed && (
          <div className={`absolute bottom-4 left-4 right-4 bg-black/80 text-white text-xs rounded-lg border border-white/20 z-50 transition-all ${
            isOutputMinimized ? 'h-10' : 'max-h-64'
          }`}>
            <div className="flex items-center justify-between p-4">
              <h3 className="font-bold">Simulation Output</h3>
              <div className="flex gap-2">
                <button
                  onClick={() => setIsOutputMinimized(!isOutputMinimized)}
                  className="px-3 py-1 bg-white/20 hover:bg-white/30 rounded text-xs font-medium transition-colors"
                >
                  {isOutputMinimized ? 'Expand' : 'Minimize'}
                </button>
                <button
                  onClick={() => setIsOutputClosed(true)}
                  className="px-3 py-1 bg-white/20 hover:bg-white/30 rounded text-xs font-medium transition-colors"
                >
                  Close
                </button>
              </div>
            </div>
            {!isOutputMinimized && (
              <div className="px-4 pb-4 overflow-auto max-h-56">
                <pre>{JSON.stringify(simulationResult, null, 2)}</pre>
              </div>
            )}
          </div>
        )}

        {/* Floating button to reopen output */}
        {simulationResult && isOutputClosed && (
          <button
            onClick={() => setIsOutputClosed(false)}
            className="absolute bottom-4 left-4 px-4 py-2 bg-black/80 text-white text-xs font-medium rounded-lg border border-white/20 hover:bg-black/90 transition-colors z-50"
          >
            View Output
          </button>
        )}
      </div>
    </TooltipProvider>
  );
};

export default Index;
