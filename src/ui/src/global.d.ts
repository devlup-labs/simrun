export {};

declare global {
  interface Window {
    api: {
      ping: () => Promise<string>;
      simulate: (projectJson: any) => Promise<any>;
    };
  }
}
