#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

/**
 * ── NETWORK MANAGER ───────────────────────────────────────────────────────
 * Handles WebServer, WebSockets, and API routing.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

class NetworkManager
{
private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    AsyncEventSource _sse;

    unsigned long _lastWsCleanup = 0;
    uint32_t _workTimeAccum = 0;
    unsigned long _lastStatsTime = 0;

    void _setupRoutes();
    String _buildStatusJson();
    void _onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

public:
    NetworkManager();

    // ── Life Cycle ──
    void begin();
    void process(); // To be called from TaskNetwork

    // ── Communication ──
    void broadcastStatus(bool force = false);
    void sendToAll(const String& msg);
    void sendRxEvent(char c); // Push one decoded RX character to all WebSocket clients + SSE

    // ── Helpers ──
    String getActiveIP();
    String getIndexHtml();
};

extern NetworkManager network;

#endif
