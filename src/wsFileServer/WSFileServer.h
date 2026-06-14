//
// WSFileServer.h
//

#pragma once
#include <ESPAsyncWebServer.h>

class WSFileServer {
public:
    static void init(AsyncWebServer &server);
    static AsyncWebSocket *socket;

private:
    static void _onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
};
