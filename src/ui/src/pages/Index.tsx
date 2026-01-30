import { ComponentToolbar } from '@/components/toolbar/ComponentToolbar';
import { ArchitectureCanvas } from '@/components/canvas/ArchitectureCanvas';
import { InspectorPanel } from '@/components/inspector/InspectorPanel';
import { CanvasHeader } from '@/components/header/CanvasHeader';
import { ConfigTabs } from '@/components/panels/ConfigTabs';
import { TooltipProvider } from '@/components/ui/tooltip';
import { useState } from "react";
import { toast } from "sonner";

const Index = () => {
  const [simulationResult, setSimulationResult] = useState<any>(null);

  const handleRunSimulation = async () => {
    const projectJson = {
      test: "demo" // replace later with real canvas export
    };

    try {
      const result = await window.api.simulate(projectJson);

      if (!result) {
        toast.error("No response from compiler");
        return;
      }

      setSimulationResult(result);
      toast.success("Simulation completed 🚀");

    } catch (err) {
      toast.error("System error: Compiler not reachable");
      console.error("Simulation failed:", err);
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
        {simulationResult && (
          <div className="absolute bottom-4 left-4 right-4 max-h-64 overflow-auto bg-black/80 text-white text-xs p-4 rounded-lg border border-white/20 z-50">
            <h3 className="font-bold mb-2">Simulation Output</h3>
            <pre>{JSON.stringify(simulationResult, null, 2)}</pre>
          </div>
        )}
      </div>
    </TooltipProvider>
  );
};

export default Index;
