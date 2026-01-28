import { ComponentToolbar } from '@/components/toolbar/ComponentToolbar';
import { ArchitectureCanvas } from '@/components/canvas/ArchitectureCanvas';
import { InspectorPanel } from '@/components/inspector/InspectorPanel';
import { CanvasHeader } from '@/components/header/CanvasHeader';
import { ConfigTabs } from '@/components/panels/ConfigTabs';
import { TooltipProvider } from '@/components/ui/tooltip';

const Index = () => {
  const handleRunSimulation = async () => {
    const projectJson = {
      test: "demo" // replace later with real canvas export
    };

    try {
      const result = await window.api.simulate(projectJson);
      console.log("Simulation response:", result);
    } catch (err) {
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
      </div>
    </TooltipProvider>
  );
};

export default Index;
