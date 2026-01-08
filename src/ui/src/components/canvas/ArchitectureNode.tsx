import { memo } from 'react';
import { Handle, Position, NodeProps } from '@xyflow/react';
import { cn } from '@/lib/utils';
import { ComponentCategory, ComponentProfile } from '@/types/architecture';
import { useArchitectureStore } from '@/store/architectureStore';
import { getComponentLogo } from '@/components/icons/ComponentLogos';

interface ArchitectureNodeData {
  label: string;
  profile: ComponentProfile;
  category: ComponentCategory;
}

export const ArchitectureNode = memo(({ id, data, selected }: NodeProps & { data: ArchitectureNodeData }) => {
  const { selectNode } = useArchitectureStore();
  const LogoComponent = getComponentLogo(data.profile);

  const categoryBorderClass: Record<ComponentCategory, string> = {
    database: 'border-gradient-database',
    cache: 'border-gradient-cache',
    api: 'border-gradient-api',
    network: 'border-gradient-network',
  };

  const categoryGlowClass: Record<ComponentCategory, string> = {
    database: 'glow-database',
    cache: 'glow-cache',
    api: 'glow-api',
    network: 'glow-network',
  };

  const categoryTextClass: Record<ComponentCategory, string> = {
    database: 'text-database',
    cache: 'text-cache',
    api: 'text-api',
    network: 'text-network',
  };

  return (
    <div
      onClick={() => selectNode(id)}
      className={cn(
        'group relative flex flex-col items-center py-4 px-5 rounded-2xl transition-all duration-300 cursor-pointer',
        'bg-node-bg/95 backdrop-blur-sm min-w-[160px]',
        categoryBorderClass[data.category],
        selected
          ? cn('ring-2 ring-primary ring-offset-2 ring-offset-canvas-bg animate-node-pulse', categoryGlowClass[data.category])
          : 'hover:shadow-2xl hover:-translate-y-0.5'
      )}
    >
      {/* Target Handle */}
      <Handle
        type="target"
        position={Position.Left}
        className="!w-3 !h-3 !-left-1.5"
      />
      
      {/* Logo */}
      <div className="mb-2">
        <LogoComponent size={48} />
      </div>
      
      {/* Label */}
      <span className="text-sm font-semibold text-foreground truncate max-w-[140px] text-center">
        {data.label}
      </span>
      
      {/* Profile badge */}
      <span className={cn(
        'text-[10px] font-mono uppercase tracking-wider mt-1',
        categoryTextClass[data.category]
      )}>
        {data.profile}
      </span>

      {/* Source Handle */}
      <Handle
        type="source"
        position={Position.Right}
        className="!w-3 !h-3 !-right-1.5"
      />
    </div>
  );
});

ArchitectureNode.displayName = 'ArchitectureNode';
