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
    AsyncWebServer server;
    AsyncWebSocket ws;
    AsyncEventSource sse;

    unsigned long lastWsCleanup = 0;
    uint32_t workTimeAccum = 0;
    unsigned long lastStatsTime = 0;

    void setupRoutes();
    void buildStatusJson(String& json);
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

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
