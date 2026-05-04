var Clay = require('./pebble-clay');

var clayConfig = [
  {
    type: 'heading',
    defaultValue: 'HappyMac'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Connection'
      },
      {
        type: 'toggle',
        messageKey: 'BT_STATUS_ENABLED',
        label: 'Show connection status',
        defaultValue: true
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Theme'
      },
      {
        type: 'select',
        messageKey: 'theme',
        label: 'Theme',
        defaultValue: 0,
        options: [
          { value: 0, label: 'Light' },
          { value: 1, label: 'Dark' },
          { value: 2, label: 'Color' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Weather'
      },
      {
        type: 'toggle',
        messageKey: 'WEATHER_ENABLED',
        label: 'Enable weather',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'WEATHER_SHOW_TEMP',
        label: 'Show temperature',
        defaultValue: true
      },
      {
        type: 'select',
        messageKey: 'WEATHER_TEMP_UNIT',
        label: 'Temperature unit',
        defaultValue: 'C',
        options: [
          { value: 'C', label: 'Celsius' },
          { value: 'F', label: 'Fahrenheit' }
        ]
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];

var clay = new Clay(clayConfig, null, { autoHandleEvents: false });
var configurationOpenPending = false;
var configurationOpenTimer = null;

function openConfiguration() {
  if (configurationOpenTimer) {
    clearTimeout(configurationOpenTimer);
    configurationOpenTimer = null;
  }
  if (!configurationOpenPending) {
    return;
  }

  configurationOpenPending = false;
  Pebble.openURL(clay.generateUrl());
}

Pebble.addEventListener('showConfiguration', function() {
  configurationOpenPending = true;
  Pebble.sendAppMessage(
    { SETTINGS_REQUEST: 1 },
    function() {
      configurationOpenTimer = setTimeout(openConfiguration, 3000);
    },
    function() {
      openConfiguration();
    }
  );
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) {
    return;
  }

  var settings = clay.getSettings(e.response);
  Pebble.sendAppMessage(
    settings,
    function() {
      console.log('Sent config data to Pebble');
    },
    function(error) {
      console.log('Failed to send config data!', JSON.stringify(error));
    }
  );
});

function fetch(url, onResponse, onError) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    onResponse(this.responseText);
  };
  xhr.onerror = function(error) {
    console.log('fetch onerror', JSON.stringify(this), JSON.stringify(error));
    if (onError) {
      onError(error);
    }
  };
  xhr.open('GET', url);
  xhr.send();
}

function fetchWeather(temperatureUnit, pos) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
      '?latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&temperature_unit=' + temperatureUnit +
      '&current_weather=true' +
      '&timezone=auto';

  fetch(url, function(res) {
    console.log('weather raw', res);
    var data = JSON.parse(res);
    if (!data || !data.current_weather) {
      return;
    }
    Pebble.sendAppMessage({
      WEATHER_TEMP: Math.round(data.current_weather.temperature),
      WEATHER_CODE: data.current_weather.weathercode
    });
  });
}

function weatherGet(unitCode) {
  var temperatureUnit = unitCode === 1 ? 'fahrenheit' : 'celsius';
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      localStorage.setItem('currentPosition', JSON.stringify(pos));
      fetchWeather(temperatureUnit, pos);
    },
    function() {
      var cached = localStorage.getItem('currentPosition');
      if (!cached) {
        return;
      }
      fetchWeather(temperatureUnit, JSON.parse(cached));
    },
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('appmessage', function(e) {
  if (!e || !e.payload) {
    return;
  }
  console.log('appmessage payload', JSON.stringify(e.payload));
  if (typeof e.payload.theme !== 'undefined') {
    clay.setSettings({ theme: e.payload.theme });
  }
  if (typeof e.payload.WEATHER_ENABLED !== 'undefined') {
    clay.setSettings({ WEATHER_ENABLED: !!e.payload.WEATHER_ENABLED });
  }
  if (typeof e.payload.WEATHER_SHOW_TEMP !== 'undefined') {
    clay.setSettings({ WEATHER_SHOW_TEMP: !!e.payload.WEATHER_SHOW_TEMP });
  }
  if (typeof e.payload.WEATHER_TEMP_UNIT !== 'undefined') {
    var unit = e.payload.WEATHER_TEMP_UNIT === 1 ? 'F' : 'C';
    clay.setSettings({ WEATHER_TEMP_UNIT: unit });
  }
  if (typeof e.payload.BT_STATUS_ENABLED !== 'undefined') {
    clay.setSettings({ BT_STATUS_ENABLED: !!e.payload.BT_STATUS_ENABLED });
  }
  if (configurationOpenPending &&
      typeof e.payload.theme !== 'undefined' &&
      typeof e.payload.WEATHER_ENABLED !== 'undefined' &&
      typeof e.payload.WEATHER_SHOW_TEMP !== 'undefined' &&
      typeof e.payload.WEATHER_TEMP_UNIT !== 'undefined' &&
      typeof e.payload.BT_STATUS_ENABLED !== 'undefined') {
    openConfiguration();
  }
  if (typeof e.payload.WEATHER_REQUEST !== 'undefined') {
    weatherGet(e.payload.WEATHER_REQUEST);
  }
});
