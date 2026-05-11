#ifndef ALERTER_H
#define ALERTER_H

#include "weather_alerts.h"
#include <ctime>
#include <set>
#include <string>

// Sends email notifications via Gmail SMTP.
//
// Two trigger sources:
//   1. WeatherCache reports the API has been down too long.
//      -> notifyApiOutage(now)
//   2. A weather alert just became active (severe weather, big temp swing, ...).
//      -> notifyWeatherAlerts(alerts, now)
//
// The class remembers what it's already sent so it doesn't re-send the
// same alert every minute. State is in-memory only; on restart we may
// re-send recent alerts once, which is acceptable.
//
// Credentials come from env vars set by systemd's EnvironmentFile:
//   GMAIL_USER          - sender Gmail address
//   GMAIL_APP_PASSWORD  - 16-char Google app password
//   ALERT_RECIPIENT     - destination email (often the same as sender)
//
// If any of these are missing, the alerter silently no-ops (logging a
// warning at startup). This lets the project run fine without alerts
// configured.
class Alerter {
public:
    // Read credentials from environment. Returns true if all three were
    // present and we're ready to send mail.
    bool configure();

    // True if configure() succeeded.
    bool isReady() const { return ready_; }

    // The sender / recipient (for the startup banner).
    const std::string& sender()    const { return gmailUser_; }
    const std::string& recipient() const { return recipient_; }

    // Fire-and-forget. Returns true on successful submission to Gmail.
    bool sendEmail(const std::string& subject, const std::string& body);

    // ---- High-level entry points ----

    // Called when the weather cache reports a sustained outage.
    // Sends one email per "outage episode" (no spam if outage continues).
    // Caller is expected to call this on every tick during the outage;
    // we de-duplicate internally.
    void notifyApiOutage(std::time_t now,
                          int consecutiveFailures,
                          std::time_t lastSuccess);

    // Called when a successful fetch comes in. Resets the outage flag
    // so the next outage will trigger a new email.
    void clearOutageState();

    // Called every tick with the currently active alerts. Sends an
    // email per new alert; remembers what's been sent.
    void notifyWeatherAlerts(const weather_alerts::AlertSet& alerts,
                              std::time_t now);

private:
    bool        ready_       = false;
    std::string gmailUser_;
    std::string gmailPass_;
    std::string recipient_;

    // Outage tracking: did we already alert on the current outage?
    bool        outageAlertSent_ = false;

    // Weather alert tracking: which alert keys have we already emailed?
    std::set<std::string> sentAlertKeys_;
};

#endif