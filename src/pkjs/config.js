const config = [
    {
        "type": "heading",
        "defaultValue": "2077"
    },
    {
        "type": "section",
        "capabilities": ["HEALTH"],
        "items": [
            {
                "type": "heading",
                "defaultValue": "Health"
            },
            {
                "type": "toggle",
                "messageKey": "PREF_SHOW_STEPS",
                "label": "Show step count",
                "defaultValue": true
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Weather"
            },
            {
                "type": "text",
                "defaultValue": "Weather data is sourced from Open-Meteo. Location is determined automatically using your device location."
            },
            {
                "type": "toggle",
                "messageKey": "PREF_SHOW_WEATHER",
                "label": "Show current weather",
                "defaultValue": true
            },
            {
                "type": "toggle",
                "messageKey": "PREF_WEATHER_METRIC",
                "label": "Use metric units (Celsius)",
                "defaultValue": true
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Alerts"
            },
            {
                "type": "toggle",
                "messageKey": "PREF_HOUR_VIBE",
                "label": "Hourly vibration",
                "defaultValue": false
            },
            {
                "type": "toggle",
                "messageKey": "PREF_DISCONNECT_ALERT",
                "label": "Disconnect alert",
                "defaultValue": true
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Top Text Customization"
            },
            {
                "type": "select",
                "id": "top-text-select",
                "label": "Presets",
                "defaultValue": "",
                "options": [
                    {
                        "value": "",
                        "label": "None"
                    },
                    {
                        "value": "PBL_%m%U%j",
                        "label": "Default: Month, week number, and day of year"
                    },
                    {
                        "value": "%Y_%b",
                        "label": "Year and short month"
                    },
                    {
                        "value": "%B",
                        "label": "Full month"
                    },
                    {
                        "value": "%G_%V",
                        "label": "ISO 8601 year and week number"
                    }
                ]
            },
            {
                "type": "input",
                "id": "top-text-input",
                "messageKey": "PREF_CUSTOM_TEXT",
                "label": "Text",
                "defaultValue": "PBL_%m%U%j",
                "attributes": {
                    "limit": 16
                }
            },
            {
                "type": "text",
                "defaultValue": "Text can be formatted with the current time by following the <a href=\"https://man7.org/linux/man-pages/man3/strftime.3.html\">strftime(3) manpage</a>. Some common examples:<ul><li>%b - abbreviated month name</li><li>%B - full month name</li><li>%j - day of the year</li><li>%m - month</li><li>%u - day of the week as a number</li><li>%U - week number</li><li>%y - two digit year</li><li>%Y - full year</li></ul>"
            }
        ]
    },
    {
        "type": "submit",
        "defaultValue": "Save"
    }
]

module.exports = config