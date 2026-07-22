Pebble.addEventListener('showConfiguration', function() {
  var settings = {
    THEME: localStorage.getItem('THEME') || '0'
  };
  Pebble.openURL(
    'https://geopebblemx.github.io/necaxa-watchface-config/#' +
    encodeURIComponent(JSON.stringify(settings))
  );
});
Pebble.addEventListener('webviewclosed', function(e) {
  if (!e.response) return;
  var settings;
  try {
    settings = JSON.parse(decodeURIComponent(e.response));
  } catch(err) {
    console.log('Invalid settings');
    return;
  }
  var theme = Number(settings.THEME);
  if (isNaN(theme) || theme < 0) theme = 0;
  if (theme > 2) theme = 2;
  localStorage.setItem('THEME', theme);
  Pebble.sendAppMessage({
    THEME: theme
  });
});
