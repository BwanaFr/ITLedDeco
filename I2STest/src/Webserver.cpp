#include <Webserver.hpp>
#include <Arduino.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include "PCM1808.hpp"

static const char* TAG = "Webserver";

extern PCM1808 pcm;

Webserver webServer;

esp_err_t Webserver::get_buffer_handler(httpd_req_t *req)
{
    Webserver* instance = static_cast<Webserver*>(req->user_ctx);
    size_t count = 0;
    const PCM1808::audio_sample_t* buf = pcm.getBuffer(count);
    if(buf){
        httpd_resp_set_type(req, "application/octet-stream");
        httpd_resp_send(req, (const char *)buf, count);
    }else{
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Empty buffer!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t Webserver::get_reset_buffer_handler(httpd_req_t *req)
{
    Webserver* instance = static_cast<Webserver*>(req->user_ctx);
    const std::string msg = "Reset done!";
    pcm.reset();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, (const char *)msg.c_str(), msg.length());
    return ESP_OK;
}

Webserver::Webserver() : server_(nullptr)
{
}

void Webserver::start()
{
    if(!server_){
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        // Start the httpd server
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        if (httpd_start(&server_, &config) == ESP_OK) {
            httpd_uri_t get_buffer_cfg_handler =
            {
                .uri       = "/buffer",
                .method    = HTTP_GET,
                .handler   = get_buffer_handler,
                .user_ctx  = this
            };
            httpd_register_uri_handler(server_, &get_buffer_cfg_handler);

            httpd_uri_t reset_buffer_cfg_handler =
            {
                .uri       = "/reset",
                .method    = HTTP_GET,
                .handler   = get_reset_buffer_handler,
                .user_ctx  = this
            };
            httpd_register_uri_handler(server_, &reset_buffer_cfg_handler);
            return;
        }
        ESP_LOGI(TAG, "Error starting server!");
        server_ = nullptr;
    }
}

void Webserver::stop()
{
    if(server_){
        httpd_stop(server_);
        server_ = nullptr;
    }
}

void Webserver::setup()
{

}
