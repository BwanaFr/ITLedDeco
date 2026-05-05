#ifndef _UPS_WEB_SERVER_H__
#define _UPS_WEB_SERVER_H__

#include <esp_http_server.h>
#include <string>
#include <map>
#include <functional>

class Webserver{
public:
    Webserver();
    virtual ~Webserver() = default;

    /**
     * Stars the Webserver
     */
    void start();

    /**
     * Stops the webserver
     */
    void stop();

    /**
     * Setup
     */
    void setup();


private:
    httpd_handle_t server_;

    //HTTP handlers
    static esp_err_t get_buffer_handler(httpd_req_t *req);                 //Handle GET
    static esp_err_t get_reset_buffer_handler(httpd_req_t *req);                 //Handle GET
};

extern Webserver webServer;
#endif