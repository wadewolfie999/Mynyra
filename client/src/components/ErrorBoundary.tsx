import { cn } from "@/lib/utils";
import { AlertTriangle, RotateCcw } from "lucide-react";
import { Component, ReactNode } from "react";

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
  error: Error | null;
}

class ErrorBoundary extends Component<Props, State> {
  constructor(props: Props) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error): State {
    return { hasError: true, error };
  }

  render() {
    if (this.state.hasError) {
      return (
        <main className="manual-state-page">
          <section
            className="manual-state-card manual-state-card--wide"
            aria-labelledby="error-title"
          >
            <div className="manual-state-content">
              <AlertTriangle className="manual-state-icon" aria-hidden="true" />

              <h1 id="error-title" className="manual-state-title">
                An unexpected error occurred.
              </h1>

              <div className="manual-error-detail">
                <pre>{this.state.error?.stack}</pre>
              </div>

              <button
                onClick={() => window.location.reload()}
                className={cn(
                  "flex items-center gap-2 px-4 py-2 rounded-lg",
                  "manual-state-action"
                )}
              >
                <RotateCcw size={16} />
                Reload Page
              </button>
            </div>
          </section>
        </main>
      );
    }

    return this.props.children;
  }
}

export default ErrorBoundary;
