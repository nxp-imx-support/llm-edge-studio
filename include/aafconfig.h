/*
 * Copyright 2026 NXP
 *
 * NXP Proprietary. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and that
 * you agree to comply with and are bound by, such license terms.  If you do
 * not agree to be bound by the applicable license terms, then you may not
 * retain, install, activate or otherwise use the software.
 *
 */

#ifndef AAFCONFIG_H
#define AAFCONFIG_H

#include <QString>

/**
 * @brief Configuration constants for AAF Connector
 *
 * This namespace contains all configuration constants used by the
 * AI Accelerator Framework connector. Modify these values to match
 * your deployment environment.
 */
namespace AAFConfig {

// Server configuration
constexpr const char* DEFAULT_SERVER_URL = "http://localhost:8000";

// Timeout configuration (in milliseconds)
constexpr int DEFAULT_TIMEOUT_MS = 300000;  // 5 minutes

// File paths - Updated to match actual deployment paths
constexpr const char* CONFIG_PATH =
    "/usr/share/llm-edge-studio/server_config.json";
constexpr const char* MODEL_PATH = "/usr/share/llm";

// Model configuration
constexpr int DEFAULT_MAX_PROMPT_SIZE = 2048;
constexpr int DEFAULT_MAX_TOKENS = 1000;
constexpr double DEFAULT_TEMPERATURE = 0.7;

// Polling intervals (in milliseconds)
constexpr int MODEL_STATUS_POLL_INTERVAL_MS = 2000;
constexpr int METRICS_REQUEST_DELAY_MS = 500;

// API endpoints
constexpr const char* ENDPOINT_CHAT_COMPLETIONS = "/v1/chat/completions";
constexpr const char* ENDPOINT_MODELS = "/v1/models";
constexpr const char* ENDPOINT_METRICS = "/metrics/";

// User agent
constexpr const char* USER_AGENT = "AAFConnector/1.0";

}  // namespace AAFConfig

#endif  // AAFCONFIG_H
