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
    type: 'submit',
    defaultValue: 'Save'
  }
];

var clay = new Clay(clayConfig);

Pebble.addEventListener('appmessage', function(e) {
  if (!e || !e.payload || typeof e.payload.theme === 'undefined') {
    return;
  }
  clay.setSettings({ theme: e.payload.theme });
});
