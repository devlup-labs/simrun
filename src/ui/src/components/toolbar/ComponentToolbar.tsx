import { useState } from 'react';
import { ChevronDown, ChevronRight, GripVertical, PanelLeftClose, PanelLeft } from 'lucide-react';
import { cn } from '@/lib/utils';
import { COMPONENT_DEFINITIONS, CATEGORY_LABELS, ComponentCategory, ComponentDefinition } from '@/types/architecture';
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip';
import { getComponentLogo } from '@/components/icons/ComponentLogos';
import { Button } from '@/components/ui/button';

interface ToolbarSectionProps {
  category: ComponentCategory;
  components: ComponentDefinition[];
  onDragStart: (component: ComponentDefinition) => void;
  collapsed?: boolean;
}

const categoryIcons: Record<ComponentCategory, string> = {
  database: '',
  cache: '',
  api: '',
  network: '',
};

const categoryGradients: Record<ComponentCategory, string> = {
  database: 'from-blue-500 to-blue-600',
  cache: 'from-amber-500 to-orange-500',
  api: 'from-emerald-500 to-green-600',
  network: 'from-violet-500 to-purple-600',
};

const ToolbarSection = ({ category, components, onDragStart, collapsed }: ToolbarSectionProps) => {
  const [isOpen, setIsOpen] = useState(true);

  if (collapsed) {
    return (
      <div className="px-2 py-1">
        {components.map((component) => {
          const LogoComponent = getComponentLogo(component.profile);
          return (
            <Tooltip key={component.id}>
              <TooltipTrigger asChild>
                <div
                  draggable
                  onDragStart={(e) => {
                    e.dataTransfer.setData('application/json', JSON.stringify(component));
                    e.dataTransfer.effectAllowed = 'copy';
                    onDragStart(component);
                  }}
                  className="w-10 h-10 flex items-center justify-center rounded-lg cursor-grab active:cursor-grabbing 
                             hover:bg-accent transition-all duration-200 mb-1"
                >
                  <LogoComponent size={24} />
                </div>
              </TooltipTrigger>
              <TooltipContent side="right" className="max-w-[200px]">
                <p className="font-medium">{component.name}</p>
                <p className="text-xs text-muted-foreground">{component.description}</p>
              </TooltipContent>
            </Tooltip>
          );
        })}
      </div>
    );
  }

  return (
    <div className="animate-fade-in">
      <button
        onClick={() => setIsOpen(!isOpen)}
        className="flex items-center gap-2 w-full px-4 py-2.5 text-sm font-medium text-muted-foreground 
                   hover:text-foreground hover:bg-accent/50 transition-all duration-200"
      >
        {isOpen ? (
          <ChevronDown className="w-4 h-4" />
        ) : (
          <ChevronRight className="w-4 h-4" />
        )}
        <span className={cn('text-sm font-semibold bg-gradient-to-r bg-clip-text text-transparent', categoryGradients[category])}>
          {categoryIcons[category]} {CATEGORY_LABELS[category]}
        </span>
      </button>

      {isOpen && (
        <div className="px-3 pb-3 space-y-2">
          {components.map((component) => {
            const LogoComponent = getComponentLogo(component.profile);
            
            return (
              <Tooltip key={component.id}>
                <TooltipTrigger asChild>
                  <div
                    draggable
                    onDragStart={(e) => {
                      e.dataTransfer.setData('application/json', JSON.stringify(component));
                      e.dataTransfer.effectAllowed = 'copy';
                      onDragStart(component);
                    }}
                    className={cn(
                      'component-card group relative flex items-center gap-3 px-3 py-3 rounded-xl cursor-grab',
                      'bg-card/50 border border-border/50',
                      'hover:border-border hover:shadow-lg hover:shadow-primary/5',
                      'active:cursor-grabbing active:scale-[0.98]',
                      category === 'database' && 'hover:glow-database hover:border-database/30',
                      category === 'cache' && 'hover:glow-cache hover:border-cache/30',
                      category === 'api' && 'hover:glow-api hover:border-api/30',
                      category === 'network' && 'hover:glow-network hover:border-network/30',
                    )}
                  >
                    <GripVertical className="w-4 h-4 text-muted-foreground/30 opacity-0 group-hover:opacity-100 
                                             transition-opacity absolute left-1" />
                    <div className="ml-3 flex-shrink-0">
                      <LogoComponent size={36} />
                    </div>
                    <div className="flex-1 min-w-0">
                      <p className="text-sm font-medium text-foreground truncate">{component.name}</p>
                      <p className="text-xs text-muted-foreground truncate">{component.description}</p>
                    </div>
                  </div>
                </TooltipTrigger>
                <TooltipContent side="right" className="max-w-[220px] glass">
                  <p className="font-semibold">{component.name}</p>
                  <p className="text-xs text-muted-foreground mt-1">{component.description}</p>
                </TooltipContent>
              </Tooltip>
            );
          })}
        </div>
      )}
    </div>
  );
};

interface ComponentToolbarProps {
  onDragStart?: (component: ComponentDefinition) => void;
}

export const ComponentToolbar = ({ onDragStart = () => {} }: ComponentToolbarProps) => {
  const [collapsed, setCollapsed] = useState(false);
  const categories: ComponentCategory[] = ['database', 'cache', 'api', 'network'];

  const groupedComponents = categories.reduce((acc, category) => {
    acc[category] = COMPONENT_DEFINITIONS.filter((c) => c.category === category);
    return acc;
  }, {} as Record<ComponentCategory, ComponentDefinition[]>);

  return (
    <div 
      className={cn(
        'h-full glass-strong flex flex-col animate-slide-in-left transition-all duration-300',
        collapsed ? 'w-16' : 'w-72'
      )}
    >
      {/* Header */}
      <div className="px-4 py-4 border-b border-border/50 flex items-center justify-between">
        {!collapsed && (
          <div className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg gradient-primary flex items-center justify-center">
              <span className="text-primary-foreground font-bold text-sm">Σ</span>
            </div>
            <div>
              <h1 className="text-lg font-bold text-foreground tracking-tight">SimRUN</h1>
              <p className="text-[10px] text-muted-foreground -mt-0.5">Architecture Designer</p>
            </div>
          </div>
        )}
        {collapsed && (
          <div className="w-8 h-8 rounded-lg gradient-primary flex items-center justify-center mx-auto">
            <span className="text-primary-foreground font-bold text-sm">Σ</span>
          </div>
        )}
        <Button
          variant="ghost"
          size="icon"
          onClick={() => setCollapsed(!collapsed)}
          className={cn('h-8 w-8 shrink-0', collapsed && 'mx-auto mt-2')}
        >
          {collapsed ? <PanelLeft className="w-4 h-4" /> : <PanelLeftClose className="w-4 h-4" />}
        </Button>
      </div>

      {/* Component Palette */}
      <div className="flex-1 overflow-y-auto py-3">
        {!collapsed && (
          <div className="px-4 py-2 mb-1">
            <span className="text-[10px] uppercase tracking-widest text-muted-foreground font-semibold">
              Components
            </span>
          </div>
        )}
        
        {categories.map((category) => (
          <ToolbarSection
            key={category}
            category={category}
            components={groupedComponents[category]}
            onDragStart={onDragStart}
            collapsed={collapsed}
          />
        ))}
      </div>

      {/* Footer */}
      {!collapsed && (
        <div className="px-4 py-3 border-t border-border/50">
          <p className="text-[10px] text-muted-foreground text-center">
            Drag components onto canvas
          </p>
        </div>
      )}
    </div>
  );
};
