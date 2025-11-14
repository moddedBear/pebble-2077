const Clay = require('pebble-clay')
const clayConfig = require('./config')

function customClay() {
    this.on(this.EVENTS.AFTER_BUILD, function() {
        const topTextSelect = this.getItemById('top-text-select')
        const topTextInput = this.getItemById('top-text-input')

        topTextSelect.on('change', function() {
            topTextInput.set(topTextSelect.get())
        })
    }.bind(this))
}

const clay = new Clay(clayConfig, customClay)

function getWeatherDescription(code) {
  const map = {
    0: "CLEAR",
    1: "CLEAR-",
    2: "PRT_CLOUDY",
    3: "OVERCAST",
    45: "FOG",
    48: "FOG",
    51: "DRIZZLE-",
    53: "DRIZZLE",
    55: "DRIZZLE+",
    56: "FRZ_DRIZ-",
    57: "FRZ_DRIZ+",
    61: "RAIN-",
    63: "RAIN",
    65: "RAIN+",
    66: "FRZ_RAIN-",
    67: "FRZ_RAIN+",
    71: "SNOW-",
    73: "SNOW",
    75: "SNOW+",
    77: "SNOW_GRAIN",
    80: "SHOWERS-",
    81: "SHOWERS",
    82: "SHOWERS+",
    85: "SNW_SHOWER-",
    86: "SNW_SHOWER+",
    95: "THNDRSTRM",
    96: "STORM_HAIL-",
    99: "STORM_HAIL+"
  };

  return map.hasOwnProperty(code)
    ? map[code]
    : `UNKNOWN: ${code}`;
}

const xhrRequest = function (url, type, callback) {
    let xhr = new XMLHttpRequest();
    xhr.onload = function () {
    callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    console.log('Got location')
    const url = `https://api.open-meteo.com/v1/forecast?latitude=${pos.coords.latitude}&longitude=${pos.coords.longitude}&current=temperature_2m,weather_code`

    xhrRequest(url, 'GET',
        (res) => {
            const data = JSON.parse(res)
            const temp = Math.round(data.current.temperature_2m)
            const weather_code = parseInt(data.current.weather_code)
            const conditions = getWeatherDescription(weather_code)

            const dictionary = {
                'TEMPERATURE': temp,
                'CONDITIONS': conditions
            }

            Pebble.sendAppMessage(dictionary,
                () => {
                    console.log('Weather sent to Pebble')
                },
                () => {
                    console.log('Error sending weather to Pebble')
                })
        }
    )
}

function locationError(err) {
    console.log('Error requesting location')
}

function getWeather() {
    console.log('Getting location for weather')
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        {timeout: 15000, maximumAge: 60000}
    );
}

Pebble.addEventListener('ready',
    () => {
        getWeather()
    }
)

Pebble.addEventListener('appmessage',
    () => {
        getWeather()
    }
)
