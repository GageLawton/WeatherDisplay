#include "alerter.h"
#include "log.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <curl/curl.h>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string envOrEmpty(const char* name) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}

// libcurl SMTP read callback: it asks us for the next chunk of the email
// message via this callback. We hand out a contiguous string a chunk at a time.
struct UploadCtx {
    const std::string* payload;
    size_t             offset;
};

size_t payloadReader(char* buffer, size_t size, size_t nmemb, void* userp) {
    auto* ctx = static_cast<UploadCtx*>(userp);
    if (!ctx || !ctx->payload) return 0;
    size_t bufSize = size * nmemb;
    size_t remaining = ctx->payload->size() - ctx->offset;
    if (remaining == 0) return 0;
    size_t toCopy = (remaining < bufSize) ? remaining : bufSize;
    std::memcpy(buffer, ctx->payload->data() + ctx->offset, toCopy);
    ctx->offset += toCopy;
    return toCopy;
}

// Format current UTC time as RFC-5322-ish date for the "Date:" header.
std::string rfc5322Date() {
    std::time_t now = std::time(nullptr);
    std::tm tm = *std::gmtime(&now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S +0000", &tm);
    return buf;
}

// Format a Unix timestamp as a human-readable local time string for the body.
std::string humanLocal(std::time_t t) {
    if (t == 0) return "(never)";
    std::tm tm = *std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tm);
    return buf;
}

} // anonymous namespace

bool Alerter::configure() {
    gmailUser_ = envOrEmpty("GMAIL_USER");
    gmailPass_ = envOrEmpty("GMAIL_APP_PASSWORD");
    recipient_ = envOrEmpty("ALERT_RECIPIENT");

    if (gmailUser_.empty() || gmailPass_.empty() || recipient_.empty()) {
        LOG_WARN("Alerter not configured (missing GMAIL_USER, GMAIL_APP_PASSWORD, or ALERT_RECIPIENT). Email alerts disabled.");
        ready_ = false;
        return false;
    }

    ready_ = true;
    LOG_INFO("Alerter ready: " << gmailUser_ << " -> " << recipient_);
    return true;
}

bool Alerter::sendEmail(const std::string& subject, const std::string& body) {
    if (!ready_) return false;

    // Build the full message (headers + blank line + body).
    std::ostringstream msg;
    msg << "Date: " << rfc5322Date() << "\r\n"
        << "From: WeatherDisplay <" << gmailUser_ << ">\r\n"
        << "To: <" << recipient_ << ">\r\n"
        << "Subject: " << subject << "\r\n"
        << "Content-Type: text/plain; charset=UTF-8\r\n"
        << "\r\n"
        << body
        << "\r\n";
    std::string payload = msg.str();

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Alerter: CURL init failed");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,           "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USERNAME,      gmailUser_.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD,      gmailPass_.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL,       (long)CURLUSESSL_ALL);

    std::string mailFrom = "<" + gmailUser_ + ">";
    std::string rcptTo   = "<" + recipient_ + ">";
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM,     mailFrom.c_str());

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, rcptTo.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT,     recipients);

    UploadCtx ctx{ &payload, 0 };
    curl_easy_setopt(curl, CURLOPT_READFUNCTION,  payloadReader);
    curl_easy_setopt(curl, CURLOPT_READDATA,      &ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD,        1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       30L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Alerter: SMTP send failed: " << curl_easy_strerror(res));
        return false;
    }
    LOG_INFO("Alerter: sent email \"" << subject << "\"");
    return true;
}

void Alerter::notifyApiOutage(std::time_t now,
                               int consecutiveFailures,
                               std::time_t lastSuccess) {
    if (!ready_) return;
    if (outageAlertSent_) return;  // already alerted on this outage

    std::ostringstream body;
    body << "WeatherDisplay has been unable to reach the weather API.\n\n"
         << "Consecutive failures: " << consecutiveFailures << "\n"
         << "Last successful fetch: " << humanLocal(lastSuccess) << "\n"
         << "Time of this notice:   " << humanLocal(now) << "\n\n"
         << "The device is otherwise running normally; only the weather\n"
         << "data is affected. SSH in to investigate when convenient.\n";

    if (sendEmail("[WeatherDisplay] Weather API outage", body.str())) {
        outageAlertSent_ = true;
    }
}

void Alerter::clearOutageState() {
    if (outageAlertSent_) {
        LOG_INFO("Alerter: API recovered, outage flag cleared");
    }
    outageAlertSent_ = false;
}

void Alerter::notifyWeatherAlerts(const weather_alerts::AlertSet& alerts,
                                   std::time_t now) {
    if (!ready_) return;

    // 1. Send for new alerts we haven't seen before.
    std::set<std::string> currentKeys;
    for (const auto& a : alerts.active) {
        std::string key = weather_alerts::keyFor(a);
        currentKeys.insert(key);
        if (sentAlertKeys_.count(key)) continue;  // already emailed this one

        std::ostringstream subj, body;
        subj << "[WeatherDisplay] " << a.headline;
        body << a.detail << "\n\n"
             << "Detected at: " << humanLocal(now) << "\n";
        if (sendEmail(subj.str(), body.str())) {
            sentAlertKeys_.insert(key);
        }
    }

    // 2. Forget keys for alerts that are no longer active, so if they
    //    come back later we re-send.
    for (auto it = sentAlertKeys_.begin(); it != sentAlertKeys_.end(); ) {
        if (!currentKeys.count(*it)) {
            it = sentAlertKeys_.erase(it);
        } else {
            ++it;
        }
    }
}