import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import { AlertCircle, Home } from "lucide-react";
import { useLocation } from "wouter";

export default function NotFound() {
  const [, setLocation] = useLocation();

  const handleGoHome = () => {
    setLocation("/");
  };

  return (
    <main className="manual-state-page">
      <Card className="manual-state-card">
        <CardContent className="manual-state-content">
          <div className="manual-state-icon" aria-hidden="true">
            <AlertCircle />
          </div>

          <p className="manual-state-code">404</p>

          <h1 className="manual-state-title">Page Not Found</h1>

          <p className="manual-state-copy">
            Sorry, the page you are looking for doesn't exist.
            <br />
            It may have been moved or deleted.
          </p>

          <div className="manual-state-actions">
            <Button onClick={handleGoHome} className="manual-state-action">
              <Home className="w-4 h-4 mr-2" />
              Go Home
            </Button>
          </div>
        </CardContent>
      </Card>
    </main>
  );
}
