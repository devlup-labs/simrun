import { useCallback, useRef } from 'react';
import {
  ReactFlow,
  Background,
  Controls,
  MiniMap,
  BackgroundVariant,
  ReactFlowProvider,
  useReactFlow,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';

import { useArchitectureStore } from '@/store/architectureStore';
import { ArchitectureNode } from './ArchitectureNode';
import { ComponentDefinition } from '@/types/architecture';
import { Boxes } from 'lucide-react';

const nodeTypes = {
  architectureNode: ArchitectureNode,
};

const CanvasInner = () => {
  const reactFlowWrapper = useRef<HTMLDivElement>(null);
  const { screenToFlowPosition } = useReactFlow();
  
  const {
    nodes,
    edges,
    onNodesChange,
    onEdgesChange,
    onConnect,
    addNode,
    selectNode,
    selectEdge,
  } = useArchitectureStore();

  const onDragOver = useCallback((event: React.DragEvent) => {
    event.preventDefault();
    event.dataTransfer.dropEffect = 'copy';
  }, []);

  const onDrop = useCallback(
    (event: React.DragEvent) => {
      event.preventDefault();

      const data = event.dataTransfer.getData('application/json');
      if (!data) return;

      const component: ComponentDefinition = JSON.parse(data);
      
      const position = screenToFlowPosition({
        x: event.clientX,
        y: event.clientY,
      });

      const newNode = {
        id: `node_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        type: 'architectureNode',
        position,
        data: {
          label: component.name,
          profile: component.profile,
          category: component.category,
        },
      };

      addNode(newNode);
    },
    [screenToFlowPosition, addNode]
  );

  const onNodeClick = useCallback(
    (_: React.MouseEvent, node: { id: string }) => {
      selectNode(node.id);
    },
    [selectNode]
  );

  const onEdgeClick = useCallback(
    (_: React.MouseEvent, edge: { id: string }) => {
      selectEdge(edge.id);
    },
    [selectEdge]
  );

  const onPaneClick = useCallback(() => {
    selectNode(null);
    selectEdge(null);
  }, [selectNode, selectEdge]);

  // Empty state
  if (nodes.length === 0) {
    return (
      <div ref={reactFlowWrapper} className="flex-1 h-full relative">
        <ReactFlow
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          onDragOver={onDragOver}
          onDrop={onDrop}
          onNodeClick={onNodeClick}
          onEdgeClick={onEdgeClick}
          onPaneClick={onPaneClick}
          nodeTypes={nodeTypes}
          fitView
          snapToGrid
          snapGrid={[20, 20]}
          defaultEdgeOptions={{
            type: 'smoothstep',
            animated: true,
          }}
          proOptions={{ hideAttribution: true }}
        >
          <Background
            variant={BackgroundVariant.Dots}
            gap={24}
            size={1.5}
            color="hsl(240 15% 12%)"
          />
          <Controls className="!bottom-4 !left-4" />
          <MiniMap
            className="!bottom-4 !right-4 !w-44 !h-28"
            nodeColor={(node) => {
              const category = node.data?.category as string;
              switch (category) {
                case 'database':
                  return 'hsl(217 91% 55%)';
                case 'cache':
                  return 'hsl(38 92% 50%)';
                case 'api':
                  return 'hsl(160 84% 39%)';
                case 'network':
                  return 'hsl(270 80% 60%)';
                default:
                  return 'hsl(240 15% 35%)';
              }
            }}
            maskColor="hsl(240 15% 5% / 0.85)"
          />
        </ReactFlow>
        
        {/* Empty State Overlay */}
        <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
          <div className="text-center animate-fade-in">
            <div className="w-20 h-20 rounded-2xl bg-card/50 border border-border/50 flex items-center justify-center mx-auto mb-6 animate-float">
              <Boxes className="w-10 h-10 text-muted-foreground/50" />
            </div>
            <h3 className="text-lg font-semibold text-foreground mb-2">Start Designing</h3>
            <p className="text-sm text-muted-foreground max-w-xs">
              Drag components from the left sidebar onto the canvas to build your architecture
            </p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div ref={reactFlowWrapper} className="flex-1 h-full">
      <ReactFlow
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onEdgesChange={onEdgesChange}
        onConnect={onConnect}
        onDragOver={onDragOver}
        onDrop={onDrop}
        onNodeClick={onNodeClick}
        onEdgeClick={onEdgeClick}
        onPaneClick={onPaneClick}
        nodeTypes={nodeTypes}
        fitView
        snapToGrid
        snapGrid={[20, 20]}
        defaultEdgeOptions={{
          type: 'smoothstep',
          animated: true,
        }}
        proOptions={{ hideAttribution: true }}
      >
        <Background
          variant={BackgroundVariant.Dots}
          gap={24}
          size={1.5}
          color="hsl(240 15% 12%)"
        />
        <Controls className="!bottom-4 !left-4" />
        <MiniMap
          className="!bottom-4 !right-4 !w-44 !h-28"
          nodeColor={(node) => {
            const category = node.data?.category as string;
            switch (category) {
              case 'database':
                return 'hsl(217 91% 55%)';
              case 'cache':
                return 'hsl(38 92% 50%)';
              case 'api':
                return 'hsl(160 84% 39%)';
              case 'network':
                return 'hsl(270 80% 60%)';
              default:
                return 'hsl(240 15% 35%)';
            }
          }}
          maskColor="hsl(240 15% 5% / 0.85)"
        />
      </ReactFlow>
    </div>
  );
};

export const ArchitectureCanvas = () => {
  return (
    <ReactFlowProvider>
      <CanvasInner />
    </ReactFlowProvider>
  );
};
